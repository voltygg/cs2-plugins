# CircleCI fallback

CircleCI mirrors the GitHub Actions CI and deployment workflows.

| Branch                    | Workflow | Jobs                                           |
| ------------------------- | -------- | ---------------------------------------------- |
| any branch except `prod`  | `ci`     | `build-test`, `lint`                           |
| `prod`                    | `deploy` | `build-package` + `runtime-image` -> `deploy`  |

## Set up CircleCI

1. **Create a GHCR token.** Use a classic PAT with `read:packages` and
   `write:packages` to publish the runtime image.

2. **Install and authenticate the CLI.**

   ```bash
   winget install CircleCI-Public.CircleCI-CLI
   circleci setup                  # paste a personal API token
   ```

3. **Populate the contexts.** From the repository root, run this in Git Bash,
   not PowerShell:

   ```bash
   ./.circleci/bootstrap.sh
   CIRCLE_ORG_ID=<uuid> ./.circleci/bootstrap.sh   # GitHub App orgs
   ```

   The script reads the deploy key from disk and visits each active server in
   `deploy/inventory.yml`, uploading each one's
   `deploy/secrets/servers/<id>/.env`. Adding a box needs no edit to the script.
   Run it again after rotating a secret; uploading a value overwrites the old
   one.

4. **Connect the project** in the CircleCI dashboard to
   `.circleci/config.yml`, if it is not connected already.

5. **Turn off Project Settings -> Advanced -> Auto-cancel redundant workflows.**
   This is a project setting, not configuration. If it stays on, a second push
   to `prod` can cancel an in-flight deploy during rsync.

## Contexts

| Context      | Variable                | What it is                                    |
| ------------ | ----------------------- | --------------------------------------------- |
| `ghcr`       | `GHCR_USERNAME`         | GitHub username used as the GHCR identity      |
| `ghcr`       | `GHCR_TOKEN`            | PAT with `read:packages` + `write:packages`    |
| `cs2-deploy` | `SSH_KEY_B64`           | base64 of the deploy private key               |
| `cs2-deploy` | `SERVER_ENV_<ID>_B64`   | base64 of that server's `.env`, one per server |

`<ID>` is the inventory id upper-cased with `-` turned into `_`, so `box-a`
becomes `SERVER_ENV_BOX_A_B64`. CircleCI environment variables do not preserve
newlines, so these values are base64-encoded. The bootstrap script removes
carriage returns before encoding.

The deploy job writes each decoded `.env` back to
`deploy/secrets/servers/<id>/.env` before invoking the CLI. `python-dotenv` loads
it with `override=False`, so the job's own `SSH_KEY_FILE` wins over the
developer-local path baked into that file.

## Run jobs manually

- Set `server` to deploy one inventory server.
- Set `dry-run` to preview rsync without changing containers.

CircleCI has no dynamic matrix. Its deploy job runs the servers returned by
`deploy.tools.cli matrix --format plain` in sequence.

## Build caches

Conan packages are keyed by `conan.lock`. ccache uses the same lock hash plus the
branch and revision, with broader restore prefixes for incremental builds.

## Validate locally

```bash
circleci config validate            # schema-check before pushing
circleci local execute --job lint   # docker-executor jobs only
```

`runtime-image` cannot run locally because it needs the machine executor and the
GHCR context.
