# Deploy

Dockerized deployment for this CS2 plugin monorepo. The active runtime is a small
GHCR image based on `joedwards32/cs2`; that image owns SteamCMD/CS2 lifecycle,
while this repo renders Compose files, plugin bundles, Metamod setup hooks, and
per-server settings.

`scripts/deploy.sh` and `scripts/start-server.sh` remain local Windows dev tools.

## Shape

```text
deploy/inventory.yml          declared plugins + real Docker hosts
deploy/Dockerfile             build + runtime stages (ghcr.io/<repo>/build CI toolchain, /cs2-runtime)
deploy/docker-compose.build.yml Linux plugin build wrapper
deploy/scripts/               operator and CI entrypoints
deploy/tools/                 Python inventory/render helpers
deploy/templates/             plugin config templates rendered per server
deploy/secrets/               per-server env template (real values in GitHub secrets)
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

This installs Docker + Compose (only if missing - it auto-skips when `docker` is
already present), creates the deploy user, opens SSH and the CS2 UDP port range,
and prepares `~/deploy/cs2`. Log out/in after bootstrapping so the deploy user's
Docker group membership is active.

## One-time: shared database

Create the app login role and one database per plugin. The Postgres isn't
publicly reachable, so `--server` is simplest: the inventory is parsed locally
and only the DDL runs on the box over SSH - nothing is copied. Add `--dry-run`
to preview the commands.

```bash
DB_PASSWORD='<app-role-pw>' PGPASSWORD='<superuser-pw>' \
  bash deploy/scripts/ensure-databases.sh --server box-a --admin-user postgres
```

Plugins apply their own schema migrations on load.

## Connect to the remote database

The database isn't exposed publicly, so reach it through an SSH tunnel. This
binds a local port (default 5433, to avoid clashing with a local postgres on
5432) to the remote 5432:

```bash
deploy/scripts/tunnel-db.sh --server box-a
# or without an inventory entry:
deploy/scripts/tunnel-db.sh --host 203.0.113.10 --identity ~/.ssh/id_deploy
```

With `--server`, the SSH user/port come from the inventory and the key path from
`SSH_KEY` in the server's `.env`, so plain `--server box-a` connects without
prompting for a password. `--identity <keyfile>` overrides it.

Then in another shell:

```bash
psql "host=127.0.0.1 port=5433 dbname=admin_system user=cs2_app"
```

Ctrl-C stops the tunnel. `--local-port`, `--db-host`, `--ssh-user`, etc. override
the defaults; see `--help`.

## Inventory + secrets

The committed inventory has no active servers so CI cannot deploy documentation
hosts. Add a real server under `servers:`:

```yaml
servers:
  - id: box-a
    host: 203.0.113.10
    environment: prod-box-a
    deploy_root: /home/steam/deploy/cs2
    runtime_image: ghcr.io/OWNER/REPO/cs2-runtime:latest
    plugins: [admin-system]
    instances:
      - { name: main, port: 27015, map: de_dust2, hostname: "CS2 Main" }
```

Provide each server's secrets as a GitHub Environment secret. Fill in the
template and paste its full contents into a secret named `SERVER_ENV` on the
GitHub Environment matching the server's `environment:`:

```bash
cp deploy/secrets/servers/box-a/.env.example /tmp/box-a.env
# edit /tmp/box-a.env, then paste its contents into the SERVER_ENV secret
```

CI writes `SERVER_ENV` to `deploy/secrets/servers/<id>/.env` at deploy time. For
local/manual deploys, keep a gitignored `.env` in that dir instead.

GitHub Environments named by `environment` provide `SSH_KEY` and `SERVER_ENV`.
If the GHCR runtime image is private, also configure a remote Docker login
outside this repo before deploying.

## Deploy

Normal path: push to `prod` or run the Deploy workflow manually. CI builds the
Linux plugin bundle, publishes `ghcr.io/<repo>/cs2-runtime:latest`, renders each
server's Compose tree, rsyncs it to `deploy_root`, and runs:

```bash
docker compose pull
docker compose up -d --remove-orphans
```

Manual path:

```bash
docker build -f deploy/Dockerfile --target runtime -t ghcr.io/OWNER/REPO/cs2-runtime:latest .
docker compose -f deploy/docker-compose.build.yml run --rm --build build
bash deploy/scripts/package-plugin.sh admin-system linux
RUNTIME_IMAGE=ghcr.io/OWNER/REPO/cs2-runtime:latest \
  bash deploy/scripts/deploy.sh --server box-a
```

Use `--dry-run` to render and preview rsync without changing containers.
