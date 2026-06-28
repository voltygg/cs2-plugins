# Deploy

Dockerized deployment for this CS2 plugin monorepo. The server runtime image is
based on `joedwards32/cs2`; this repo renders Compose files, plugin bundles,
Metamod setup hooks, and per-server plugin settings.

`scripts/deploy.sh` and `scripts/start-server.sh` remain local Windows dev tools.

## Shape

```text
deploy/inventory.yml          declared plugins + Docker hosts
deploy/Dockerfile             build + runtime stages
deploy/docker-compose.build.yml Linux plugin build wrapper
deploy/scripts/bootstrap-host.sh one-time Ubuntu host bootstrap
deploy/tools/cli.py           Docker/VPS deploy CLI
deploy/tools/                 inventory/render helpers
deploy/templates/             rendered config and pre-hook templates
deploy/secrets/               per-server env template (real values in GitHub secrets)
```

Published images use only `:latest`:

```text
ghcr.io/<repo>/cs2-plugin-toolchain:latest
ghcr.io/<repo>/cs2-server-runtime:latest
```

Each CS2 instance is one container with its own persistent
`/home/steam/cs2-dedicated` bind mount. The rendered `pre.sh` hook copies plugin
files into the live CS2 tree, installs Metamod if needed, and patches
`gameinfo.gi` before launch.

## One-time: Docker host

On a fresh Ubuntu box:

```bash
sudo bash deploy/scripts/bootstrap-host.sh
# already have Docker? skip reinstalling it:
sudo bash deploy/scripts/bootstrap-host.sh --skip-docker
```

This installs Docker + Compose when missing, creates the deploy user, opens SSH
and the CS2 UDP port range, and prepares `~/deploy/cs2`.

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
    deploy_root: /home/steam/deploy/cs2
    plugins: [admin-system]
    instances:
      - { name: main, port: 27015, map: de_dust2, hostname: "CS2 Main" }
```

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
renders each server's Compose tree, rsyncs it to `deploy_root`, and runs:

```bash
docker compose pull
docker compose up -d --remove-orphans
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
