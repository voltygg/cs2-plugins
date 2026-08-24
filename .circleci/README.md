# CircleCI

CircleCI builds, checks, packages, and deploys the plugin collection. The
pipeline selects one of two workflows:

| Workflow | Runs when | Jobs |
| --- | --- | --- |
| `ci` | `prod` is false | Build and test, then lint |
| `deploy` | `prod` is true | Build the package and runtime image, then deploy |

## Set up the project

1. Create the project in CircleCI and connect this repository.
2. Create contexts named `ghcr` and `cs2-deploy`.
3. Add the required environment variables described below.
4. Disable CircleCI's automatic cancellation of redundant workflows. A canceled
   deployment can leave only part of a server fleet updated.
5. Validate the configuration:

   ```bash
   circleci config validate .circleci/config.yml
   bash -n .circleci/bootstrap.sh
   ```

The setup script is [`bootstrap.sh`](bootstrap.sh). On Windows, run it from Git
Bash. Set `CIRCLE_ORG_ID` when the CircleCI CLI cannot infer the organization.

## Contexts and secrets

### `ghcr`

Provide credentials that can publish the runtime image to GitHub Container
Registry:

- `GHCR_USERNAME`
- `GHCR_TOKEN`

### `cs2-deploy`

Provide SSH access and server environment data:

- `DEPLOY_SSH_KEY` contains the private SSH key.
- `SERVER_ENV_<ID>_B64` contains a base64-encoded environment file for one
  inventory server.
- `SERVER_ENV` is the shared fallback when no server-specific value exists.

The deployment resolves environment data in this order:
`SERVER_ENV_<ID>_B64`, `SERVER_ENV`, then the inventory server's configured
environment file. Keep the stable inventory server IDs aligned with the secret
names.

## Pipeline parameters

| Parameter | Default | Purpose |
| --- | --- | --- |
| `prod` | `false` | Select the production deployment workflow |
| `server` | empty | Limit deployment to one inventory server ID |
| `dry-run` | `false` | Show deployment actions without applying them |

Use a dry run before changing deployment inventory or server selection.

## Caches

The pipeline keys the Conan cache on the dependency lock state. Ccache restore
keys run from most to least specific: revision, then branch, then lock. An exact
hit wins, and an older compatible entry is used when there is no exact hit.

## Local checks

Run the same repository checks before pushing:

```bash
uv sync
uv run poe bootstrap
uv run poe lint
```

For deployment behavior, inventory, and recovery procedures, see
[`deploy/README.md`](../deploy/README.md).
