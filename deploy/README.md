# Deployment

The deployment tools build and operate Docker-based CS2 servers derived from
`joedwards32/cs2`. They package plugins, render per-instance Compose services,
install Metamod, synchronize remote files, and manage server updates.

`uv run poe deploy` and `uv run poe start-server` are local Windows
development commands. The commands in this guide manage remote Linux hosts.

## How deployment is organized

```text
deploy/
  inventory.yml                   Hosts, instances, plugins, and databases
  Dockerfile                      Build and runtime image stages
  docker-compose.build.yml        Linux build container
  scripts/bootstrap-host.sh       One-time Ubuntu host setup
  secrets/servers/<id>/           Gitignored local environment files
  templates/                      Compose, pre-hook, and plugin templates
  tools/                          Python deployment CLI
```

CI publishes one moving runtime tag:

```text
ghcr.io/<repo>/cs2-server-runtime:latest
```

Every CS2 instance has its own container and `csgo/addons` tree. Instances on
the same host share the SteamCMD-managed game installation:

```text
/home/steam/cs2/server                         shared host game installation
/home/steam/cs2/deploy                         generated deployment files
/home/steam/cs2/deploy/instances/<name>/addons per-instance addons
```

The shared host installation is mounted at
`/home/steam/cs2-dedicated` inside each container. An instance-specific addons
tree is then mounted over its `csgo/addons` directory, so one host can run
different plugin sets.

Before launch, `pre.sh` copies the instance bundle, installs or refreshes
Metamod, and patches `gameinfo.gi`. Synchronization does not use `--delete`,
so deployed Metamod files remain intact. The hook checks
`addons/metamod/.mms-build` on every launch and upgrades when the current
snapshot differs. Set `MMS_URL` to pin a build or `MMS_BASE` to use another
mirror.

## Prepare a host

Run the bootstrap script on a fresh Ubuntu server:

```bash
sudo bash deploy/scripts/bootstrap-host.sh
sudo bash deploy/scripts/bootstrap-host.sh --skip-docker
```

Use `--skip-docker` when Docker is already installed. The script prepares the
deployment user and directories, installs Docker and Compose when requested,
and opens SSH and the CS2 UDP port range.

## Configure inventory

Keep non-secret topology in [`inventory.yml`](inventory.yml):

```yaml
servers:
  - id: box-a
    host: 203.0.113.10
    environment: prod-box-a
    cs2_root: /home/steam/cs2
    deploy_root: /home/steam/cs2/deploy
    plugins: [admin-system]
    instances:
      - name: main
        port: 27015
        map: de_dust2
        hostname: "CS2 Main"
      - name: retake
        port: 27016
        map: de_mirage
        hostname: "CS2 Retake"
        plugins: [admin-system, retake-system]
```

An instance-level `plugins` list replaces the server default; it does not
extend it. Every referenced plugin must also exist in the inventory's top-level
`plugins` map so CI packages it. A plugin without a `database` key, such as
`bhop: {}`, does not require database variables and is skipped by
`deploy-dbs`.

The renderer sets the admin system's `server.tag` to
`<server-id>-<instance-name>` and `server.name` to the instance hostname.
Because per-server admin grants reference this tag, server IDs and instance
names must remain stable.

## Configure secrets

Inventory owns hosts, ports, paths, image names, plugins, instances, and
database names. Environment files own values such as:

- `SSH_KEY_FILE`
- `DB_PASSWORD` and `PGPASSWORD`
- `GSLT_*` and `RCON_*`
- `CHEAT_API_KEY`

For local deployment, copy the server template and keep the resulting file
gitignored:

```bash
cp deploy/secrets/servers/box-a/.env.example deploy/secrets/servers/box-a/.env
```

For GitHub Actions, store the complete environment-file content in a GitHub
Environment secret named `SERVER_ENV`. For CircleCI, use its base64 form in
`SERVER_ENV_<ID>_B64`. The CLI resolves environment data in this order:

1. `SERVER_ENV_<ID>_B64`
2. `SERVER_ENV`
3. `deploy/secrets/servers/<id>/.env`

The private deployment key is separate: use `SSH_KEY` in GitHub or
`SSH_KEY_B64` in CircleCI. Local files should point to a key with
`SSH_KEY_FILE` and must not embed `SSH_KEY`.

## Prepare plugin databases

Create the application role and one database for each database-backed plugin:

```bash
DB_PASSWORD='<app-role-password>' PGPASSWORD='<postgres-password>' \
  uv run poe deploy-dbs --server box-a --admin-user postgres
```

The command reads local inventory and executes the DDL over SSH. Plugins apply
their own schema migrations when loaded.

To reach PostgreSQL through SSH:

```bash
uv run poe deploy-tunnel --server box-a
uv run poe deploy-tunnel --host 203.0.113.10 --identity ~/.ssh/id_deploy
```

Connect from another shell:

```bash
psql "host=127.0.0.1 port=5433 dbname=admin_system user=cs2_app"
```

Press Ctrl-C to close the tunnel. Use `--local-port`, `--db-host`,
`--ssh-user`, and related options to override defaults.

## Deploy

Push to `prod` or start the Deploy workflow manually. CI:

1. Builds the Linux plugin bundle.
2. Publishes `cs2-server-runtime:latest`.
3. Renders each host's Compose tree.
4. Synchronizes it to `deploy_root`.
5. Pulls the runtime image.
6. Starts instances one at a time and waits for health.
7. Runs `docker image prune -f` to remove the dangling image left when
   `:latest` moves.

Manual equivalent:

```bash
docker build -f deploy/Dockerfile --target runtime \
  -t ghcr.io/voltygg/cs2-plugins/cs2-server-runtime:latest .
docker compose -f deploy/docker-compose.build.yml run --rm --build build
uv run poe deploy-package admin-system linux
uv run poe deploy-server --server box-a
```

Use `uv run poe deploy-server --server box-a --dry-run` to render and preview
the synchronization without changing containers.

## Update CS2

SteamCMD runs when a container starts. A running server therefore does not
receive Valve updates, and a normal deployment may leave it untouched when the
`:latest` image has not changed. Restart instances one at a time:

```bash
uv run poe deploy-update --server box-a
uv run poe deploy-update --server box-a --dry-run
```

The update command waits for SteamCMD between instances. The dry run only
prints the SSH commands.

## Remove a deployment

Preview cleanup:

```bash
uv run poe deploy-cleanup --server box-a --dry-run
```

Then remove this repository's Compose stack, generated deployment files,
shared CS2 installation, and runtime images:

```bash
uv run poe deploy-cleanup --server box-a --yes
```

Cleanup does not remove Docker, PostgreSQL, firewall rules, the `steam` user,
GitHub packages, or plugin databases.
