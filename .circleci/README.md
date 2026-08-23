# CircleCI

Mirror of the GitHub Actions workflows so either provider can run CI/CD when the
other is out of minutes. `ci.yml` maps to the `ci` workflow, `build-toolchain.yml`
to `toolchain`, `deploy.yml` to `deploy`.

| Branch / trigger         | Workflow    | Jobs                                        |
| ------------------------ | ----------- | ------------------------------------------- |
| any branch except `prod` | `ci`        | `build-test`, `lint`                        |
| push to `main`           | `toolchain` | `build-toolchain` (self-skips unless toolchain inputs changed) |
| push to `prod`           | `deploy`    | `build-package` + `runtime-image` -> `deploy` |
| Trigger Pipeline         | `toolchain` | when `force-toolchain` is `true`            |

## Setup

1. **Create a GHCR token.** CircleCI has no `GITHUB_TOKEN` equivalent, so this is
   a classic PAT with `read:packages` + `write:packages` (fine-grained tokens do
   not work with GHCR). Read access is what lets the `toolchain` executor pull
   `cs2-plugin-toolchain:latest`, so CI cannot start without it.

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

- **Rebuild the toolchain image:** Trigger Pipeline with `force-toolchain` =
  `true`. Otherwise `build-toolchain` halts itself unless `deploy/Dockerfile`,
  `conanfile.py`, `conan.lock`, or this config changed.
- **Deploy one server:** set the `server` parameter to an inventory id.
- **Preview a deploy:** set `dry-run` = `true` (rsync only, no compose).

CircleCI has no dynamic matrix, so the deploy job loops over
`deploy.tools.cli matrix --format plain` sequentially instead of fanning out one
job per server the way `deploy.yml` does.

## ccache

Keys mirror the GHA cache so both providers share a lineage: toolchain
fingerprint, then branch, then revision. CircleCI caches are immutable, so the
revision suffix plays the role of GHA's per-run key and the two shorter prefixes
are the restore fallbacks.

`CCACHE_SLOPPINESS=pch_defines` and `CCACHE_DEPEND=1` are set on the `toolchain`
executor and are not optional - without both, every PCH-using translation unit is
a permanent cache miss.

## Local checks

```bash
circleci config validate            # schema-check before pushing
circleci local execute --job lint   # docker-executor jobs only
```

`build-toolchain` and `runtime-image` cannot run locally: they need the machine
executor and contexts.
