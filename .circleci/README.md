# CircleCI

CircleCI is the fallback for the GitHub Actions CI workflow. On every push it
builds and tests the Linux plugins and lints the build tooling. It does not
deploy; deploys run only in GitHub Actions (`.github/workflows/deploy.yml`).

## Set up the project

1. Create the project in CircleCI and connect this repository.
2. Validate the configuration:

   ```bash
   circleci config validate .circleci/config.yml
   ```

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
