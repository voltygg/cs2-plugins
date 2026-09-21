---
name: deploy-test
description: Build the Linux plugins (and the voltmod checkout when it has changes) in a local Docker copy of the CI container and deploy them straight to a remote server, skipping the voltmod release -> CI -> Deploy pipeline. Use for "quick deploy", "deploy to the test server", "try it on prod", "test this on panel-a/box-a", "skip the pipeline".
argument-hint: "[server-id] (default: panel-a)"
---

# Quick deploy for testing

The full path to a server is a voltmod release tag (~7 min), a relock, and the Deploy
workflow (~10 min). This skips all of it: build the `.so` files here, stage `package/`,
and run the same `deploy-server` command CI runs. Nothing is committed, tagged or pushed.

What lands on the server is **not in git**. The next Deploy workflow run overwrites it, and
a change that tested well still ships through the `commit` skill and, for the framework,
the `release` skill.

## 1. Build and package

Needs Docker Desktop running. From the repository root, in Bash or PowerShell:

```bash
uv run python .claude/skills/deploy-test/scripts/linux_build.py            # add --test for CTest
```

It runs the `build` service of `scripts/docker-compose.yml`: the image (the CI toolchain on
the sniper SDK) is built once, then `voltmod build linux-steamrt-release` and
`deploy.tools.cli package` run inside it.
The output is `package/host` plus `package/<plugin>` for every inventory plugin.

| `--framework` | Links against | When |
| --- | --- | --- |
| `auto` (default) | the checkout if `voltmod` is dirty or not exactly at the `v<version>` tag `conan.lock` pins, else the locked release | almost always |
| `checkout` | `voltmod`, registered editable inside the container and compiled first | force it |
| `locked` | the release in `conan.lock`, downloaded from the remote | reproduce what CI would ship |

The first line of output says which one it picked. The host always ships with the
plugins, so a framework change reaches the server in the same deploy.

State lives in three Docker volumes, so later runs are incremental (a plugin-only change
is about a minute): `cs2-plugins-linux-cache` (Conan home, ccache, venvs),
`cs2-plugins-linux-build` and `cs2-plugins-linux-framework` (the two `build/linux-steamrt-release`
trees). The container has its own Conan home: the Windows editable registration and
`conan.lock` are not touched. Run it in the background; the first run compiles the
dependencies and takes much longer.

## 2. Deploy

The target is the server id passed as the skill argument (`$ARGUMENTS`), or `panel-a` when
none is given. It must be an id from `deploy/inventory.yml`; substitute it for `panel-a` below.
Always pass `--server`; without it every enabled server is deployed.
`deploy/secrets/<id>/.env` must exist.

```bash
uv run poe rcon "status" --server panel-a              # who is online - the deploy restarts the server
uv run poe deploy-server --server panel-a --dry-run    # reads only
uv run poe deploy-server --server panel-a
```

`panel-a` is the live server. If `status` shows human players, tell the user and wait for
a go-ahead before the real deploy. A panel server is stopped, uploaded to and started; a
Docker host recreates each instance. Settings are rendered from the inventory exactly as in CI.

## 3. Verify

The server needs a minute to boot before RCON answers.

```bash
uv run poe rcon "meta list" "volt list" --server panel-a
```

`meta list` must show the host, `volt list` every plugin of the instance. A plugin missing
from `volt list` after a framework change usually means a `HostAbiVersion` mismatch: the
host and plugins did not come from the same build, so rerun step 1 and redeploy. Then
test the behaviour itself with the `rcon-debug` skill.

## Common failures

- **`docker: error during connect`** - Docker Desktop is not running.
- **A header is found here but not in CI** - the Windows bind mount is case-insensitive,
  so a mis-cased include still compiles. CI stays the judge of that.
- **`locked` cannot resolve `voltmod/<version>#<revision>`** - the lock names a local
  relock that was never released. Use `--framework checkout`.
- **The toolchain changed** (new compiler package, new uv) - `--rebuild-image`.
- **A build tree is wedged** - `docker volume rm cs2-plugins-linux-build cs2-plugins-linux-framework`;
  the dependency cache survives in `cs2-plugins-linux-cache`.

## Report

State which framework the build linked, the server deployed, the `volt list` response, and
that the server now runs uncommitted binaries until the next pipeline deploy.
