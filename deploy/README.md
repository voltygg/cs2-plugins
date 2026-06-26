# Deploy

Dockerized deployment for this CS2 plugin monorepo. The active runtime is a small
GHCR image based on `joedwards32/cs2`; that image owns SteamCMD/CS2 lifecycle,
while this repo renders Compose files, plugin bundles, Metamod setup hooks, and
per-server settings.

`scripts/deploy.sh` and `scripts/start-server.sh` remain local Windows dev tools.

## Shape

```text
deploy/inventory.yml          declared plugins + real Docker hosts
deploy/Dockerfile             build image + ghcr.io/<repo>/cs2-runtime
deploy/docker-compose.build.yml Linux plugin build wrapper
deploy/scripts/               operator and CI entrypoints
deploy/tools/                 Python inventory/render helpers
deploy/templates/             plugin config templates rendered per server
deploy/secrets/               encrypted per-server env files
```

Each CS2 instance is one container with its own persistent
`/home/steam/cs2-dedicated` bind mount. The rendered `pre.sh` hook copies plugin
files into the live CS2 tree, installs Metamod if needed, and patches
`gameinfo.gi` before launch.

## One-time: Docker host

On a fresh Ubuntu box:

```bash
sudo bash deploy/scripts/bootstrap-host.sh
```

This installs Docker + Compose, creates the deploy user, opens SSH and the CS2
port range, and prepares `/opt/cs2`. Log out/in after bootstrapping so the deploy
user's Docker group membership is active.

## One-time: shared database

Create the app login role and one database per plugin:

```bash
APP_DB_PASSWORD='<app-role-pw>' PGPASSWORD='<superuser-pw>' \
  bash deploy/scripts/ensure-databases.sh --admin-user postgres
```

Plugins apply their own schema migrations on load.

## Connect to the remote database

The database isn't exposed publicly, so reach it through an SSH tunnel. This
binds a local port (default 5433, to avoid clashing with a local postgres on
5432) to the remote 5432:

```bash
deploy/scripts/tunnel-db.sh --server box-a
# or without an inventory entry:
deploy/scripts/tunnel-db.sh --host 203.0.113.10 --db-host localhost
```

Then in another shell:

```bash
psql "host=127.0.0.1 port=5433 dbname=admin_system user=cs2_app"
```

Ctrl-C stops the tunnel. `--local-port`, `--db-host`, `--ssh-user`, etc. override
the inventory; see `--help`.

## Inventory + secrets

The committed inventory has no active servers so CI cannot deploy documentation
hosts. Add a real server under `servers:`:

```yaml
servers:
  - id: box-a
    host: 203.0.113.10
    environment: prod-box-a
    deploy_root: /opt/cs2/admin-system
    runtime_image: ghcr.io/OWNER/REPO/cs2-runtime:latest
    plugins: [admin-system]
    instances:
      - { name: main, port: 27015, map: de_dust2, hostname: "CS2 Main" }
```

Create encrypted per-server env:

```bash
cp deploy/secrets/servers/box-a/.env.example deploy/secrets/servers/box-a/.env
sops --filename-override deploy/secrets/servers/box-a/.sops.env \
  -e deploy/secrets/servers/box-a/.env > deploy/secrets/servers/box-a/.sops.env
rm deploy/secrets/servers/box-a/.env
```

GitHub Environments named by `environment` provide `SSH_KEY` and
`SOPS_AGE_KEY`. If the GHCR runtime image is private, also configure a remote
Docker login outside this repo before deploying.

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
