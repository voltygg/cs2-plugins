# CircleCI

Mirror of the GitHub Actions workflows so either provider can run CI/CD when the
other is out of minutes.

| Branch                    | Workflow | Jobs                                           |
| ------------------------- | -------- | ---------------------------------------------- |
| any branch except `prod`  | `ci`     | `build-test`, `lint`                           |
| `prod`                    | `deploy` | `build-package` + `runtime-image` -> `deploy`  |

## Setup

1. **Create a GHCR token.** CircleCI uses a classic PAT with `read:packages` and
   `write:packages` to publish the runtime image.

2. **Install the CLI and authenticate.**

   ```bash
   winget install CircleCI-Public.CircleCI-CLI
   circleci setup                  # paste a personal API token
   ```

3. **Populate the contexts.** From the repo root, in Git Bash (not PowerShell):

   ```bash
   ./.circleci/bootstrap.sh
   CIRCLE_ORG_ID=<uuid> ./.circleci/bootstrap.sh   # GitHub App orgs
   ```

   It reads the deploy key off disk and walks the active servers in
   `deploy/inventory.yml`, uploading each one's
   `deploy/secrets/servers/<id>/.env`. Adding a box needs no edit to the script.
   Re-run after rotating anything - storing a secret overwrites it.

4. **Connect the project** in the CircleCI dashboard against
   `.circleci/config.yml`, if it is not connected already.

5. **Project Settings -> Advanced -> Auto-cancel redundant workflows: OFF.** This
   is a project setting, not config. Left on, a second push to `prod` cancels an
   in-flight deploy partway through an rsync.

## Contexts

| Context      | Variable                | What it is                                    |
| ------------ | ----------------------- | --------------------------------------------- |
| `ghcr`       | `GHCR_USERNAME`         | GitHub username used as the GHCR identity      |
| `ghcr`       | `GHCR_TOKEN`            | PAT with `read:packages` + `write:packages`    |
| `cs2-deploy` | `SSH_KEY_B64`           | base64 of the deploy private key               |
| `cs2-deploy` | `SERVER_ENV_<ID>_B64`   | base64 of that server's `.env`, one per server |

`<ID>` is the inventory id upper-cased with `-` turned into `_`, so `box-a`
becomes `SERVER_ENV_BOX_A_B64`. These are base64 because CircleCI env vars do not
preserve newlines; the bootstrap script strips CR before encoding so a CRLF
checkout cannot corrupt the file the server receives.

The deploy job writes each decoded `.env` back to
`deploy/secrets/servers/<id>/.env` before invoking the CLI. `python-dotenv` loads
it with `override=False`, so the job's own `SSH_KEY_FILE` wins over the
developer-local path baked into that file.

## Manual runs

- Set `server` to deploy one inventory server.
- Set `dry-run` to preview rsync without changing containers.

CircleCI has no dynamic matrix, so the deploy job loops over
`deploy.tools.cli matrix --format plain` sequentially instead of fanning out one
job per server the way `deploy.yml` does.

## Build caches

Conan packages are keyed by `conan.lock`. ccache uses the same lock hash plus the
branch and revision, with broader restore prefixes for incremental builds.

## Local checks

```bash
circleci config validate            # schema-check before pushing
circleci local execute --job lint   # docker-executor jobs only
```

`runtime-image` cannot run locally because it needs the machine executor and GHCR
context.
