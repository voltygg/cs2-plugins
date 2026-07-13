# Deploy

Dockerized deployment for this CS2 plugin monorepo. The server runtime image is
based on `joedwards32/cs2`; this repo renders Compose files, plugin bundles,
Metamod setup hooks, and per-server plugin settings.

`uv run poe deploy` (the `local` subcommand) and `scripts/start_server.py` remain
local Windows dev tools.

## Shape

```text
deploy/inventory.yml          declared plugins + Docker hosts
deploy/Dockerfile             build + runtime stages
deploy/docker-compose.build.yml Linux plugin build wrapper
deploy/scripts/bootstrap-host.sh one-time Ubuntu host bootstrap
deploy/tools/                 deploy CLI package (python -m deploy.tools.cli)
deploy/templates/             rendered config, pre-hook, and compose service templates
deploy/templates/compose.service.yml one CS2 service block, filled per instance
deploy/secrets/               per-server env template (real values in GitHub secrets)
```

Published images use only `:latest`:

```text
ghcr.io/<repo>/cs2-plugin-toolchain:latest
ghcr.io/<repo>/cs2-server-runtime:latest
```

Each CS2 instance is one container. Containers on the same VPS share one
persistent CS2 install mounted at `/home/steam/cs2-dedicated`; generated deploy
files stay separate under `/home/steam/cs2/deploy`.

```text
/home/steam/cs2/deploy                       generated Compose/env/bundles/pre-hook files
/home/steam/cs2/deploy/instances/<name>/addons  per-instance live csgo/addons tree
/home/steam/cs2/server                       shared SteamCMD-managed CS2 install
```

Game files are shared, but each instance gets its own `csgo/addons` tree bind-
mounted over the shared install, so instances on one box can run different plugin
sets. The rendered `pre.sh` hook copies the instance's plugin bundle into its
addons tree, installs or refreshes Metamod, and patches the shared `gameinfo.gi`
before launch. The addons dir is runtime state under `deploy_root`; rsync (no
`--delete`) leaves it untouched on redeploy so Metamod persists.

Metamod is re-checked on every launch against the build recorded in
`addons/metamod/.mms-build` and reinstalled when the latest snapshot differs - a
CS2 update can retire symbols an older Metamod links against, leaving it unable
to load. Set `MMS_URL` to pin a build, `MMS_BASE` to change the mirror.

## One-time: Docker host

On a fresh Ubuntu box:

```bash
sudo bash deploy/scripts/bootstrap-host.sh
# already have Docker? skip reinstalling it:
sudo bash deploy/scripts/bootstrap-host.sh --skip-docker
```

This installs Docker + Compose when missing, creates the deploy user, opens SSH
and the CS2 UDP port range, and prepares `~/cs2/deploy` and `~/cs2/server`.

## One-time: shared database

Create the app login role and one database per plugin. The Postgres is not
publicly reachable, so `--server` is simplest: the inventory is parsed locally
and the DDL runs on the box over SSH.

```bash
DB_PASSWORD='<app-role-pw>' PGPASSWORD='<superuser-pw>' \
  uv run poe deploy-dbs --server box-a --admin-user postgres
```

Plugins apply their own schema migrations on load.

## Connect to the remote database

The database is reached through an SSH tunnel:

```bash
uv run poe deploy-tunnel --server box-a
# or without an inventory entry:
uv run poe deploy-tunnel --host 203.0.113.10 --identity ~/.ssh/id_deploy
```

Then in another shell:

```bash
psql "host=127.0.0.1 port=5433 dbname=admin_system user=cs2_app"
```

Ctrl-C stops the tunnel. Use `--local-port`, `--db-host`, `--ssh-user`, etc. to
override defaults.

## Inventory + secrets

Keep non-secret server topology in `inventory.yml`:

```yaml
servers:
  - id: box-a
    host: 203.0.113.10
    environment: prod-box-a
    cs2_root: /home/steam/cs2
    deploy_root: /home/steam/cs2/deploy
    plugins: [admin-system]          # default plugin set for this box
    instances:
      - { name: main, port: 27015, map: de_dust2, hostname: "CS2 Main" }
      # Override per instance; its own `plugins` replaces the server default:
      - { name: retake, port: 27016, map: de_mirage, hostname: "CS2 Retake", plugins: [admin-system, retake-system] }
```

An instance's own `plugins:` replaces the server default (it does not extend it).
Every plugin referenced must still be declared under the top-level `plugins:` map
so CI packages it. Plugins without a `database:` key (e.g. `bhop: {}`) are DB-less:
rendering skips the `DB_*` requirements for them and `deploy-dbs` skips creating a
database.

Each instance's rendered admin-system settings automatically get a server
identity: `server.tag = <box>-<instance>` (e.g. `box-a-main`) and `server.name`
from the instance `hostname`. That tag keys per-server admin grants
(`admin_server_groups`) in the shared database, so treat box ids and instance
names as **stable** - renaming one orphans the grants that reference its tag.

`inventory.yml` owns non-secret topology: hosts, ports, deploy roots, image refs,
plugins, instances, and database names. Local server `.env` files own secrets
and local file paths such as `SSH_KEY_FILE`, `DB_PASSWORD`, `PGPASSWORD`, `GSLT_*`,
`RCON_*`, and `CHEAT_API_KEY`.

Fill the template and paste its full contents into a GitHub Environment secret
named `SERVER_ENV` on the environment matching the server's `environment:`:

```bash
cp deploy/secrets/servers/box-a/.env.example /tmp/box-a.env
```

CI writes `SERVER_ENV` to `deploy/secrets/servers/<id>/.env` at deploy time. For
local/manual deploys, keep a gitignored `.env` in that dir instead. In GitHub
Actions, keep the deploy private key in the separate `SSH_KEY` Environment
secret; `SERVER_ENV` is only the env-file content. Local `.env` files should use
`SSH_KEY_FILE=/path/to/key`, not `SSH_KEY`.

## Deploy

Normal path: push to `prod` or run the Deploy workflow manually. CI builds the
Linux plugin bundle, publishes `ghcr.io/<repo>/cs2-server-runtime:latest`,
renders each server's Compose tree, rsyncs it to `deploy_root`, pulls the
runtime image, and starts CS2 instance services one at a time. After the
services come up healthy it runs `docker image prune -f` on the box, removing
the dangling `<none>` image left behind each time the `:latest` tag moves:

```bash
docker compose pull
docker compose up -d cs2-main
```

Manual path:

```bash
docker build -f deploy/Dockerfile --target runtime \
  -t ghcr.io/m9snoi-net/cs2-plugins/cs2-server-runtime:latest .
docker compose -f deploy/docker-compose.build.yml run --rm --build build
uv run poe deploy-package admin-system linux
uv run poe deploy-server --server box-a
```

Use `--dry-run` with `deploy-server` to render and preview rsync without changing
containers.

## Updating / staying current

SteamCMD runs only when a container starts, so a long-running instance never
picks up a Valve update and joins fail with "client out of date". `deploy-server`
won't fix it (an unchanged `:latest` image makes `up -d` a no-op) - the reliable
trigger is a **restart**. When clients report being out of date, restart each
instance one at a time (the command waits for SteamCMD between instances):

```bash
uv run poe deploy-update --server box-a
uv run poe deploy-update --server box-a --dry-run   # print the SSH commands only
```

## Cleanup

Remove this repo's CS2 Docker stack, generated deploy files, shared CS2 install,
and runtime images:

```bash
uv run poe deploy-cleanup --server box-a --yes
```

Preview first:

```bash
uv run poe deploy-cleanup --server box-a --dry-run
```

Cleanup does not remove Docker, Postgres, firewall rules, the `steam` user,
GitHub packages, or plugin databases.
