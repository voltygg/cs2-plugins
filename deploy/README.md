# Deployment

```bash
uv run poe deploy package                             # stage the host and plugins under package/
uv run poe deploy push [--server ID] [--dry-run]      # install plugins and settings, restart
uv run poe deploy restart [--server ID] [--dry-run]   # restart so CS2 updates
uv run poe rcon "volt list" [--server ID] [--instance NAME]
uv run poe deploy tunnel --server ID                  # Docker hosts: database on 127.0.0.1:5433
uv run poe deploy cleanup ID --yes                    # Docker hosts: remove the deployment
```

Without `--server`, `deploy push` and `deploy restart` act on every enabled server; `rcon` and
`deploy tunnel` need a single enabled server or an explicit `--server`. `--instance` is required
when a server runs more than one instance.

Two server kinds:

| Kind | What it is | How the CLI reaches it |
| --- | --- | --- |
| `panel` | A Pterodactyl panel server | the panel's client API - no SSH, no Docker |
| `docker` | A Linux machine you control, one container per instance from `joedwards32/cs2` | SSH |

For a local server use `uv run poe run <plugin>`; see
[Local development](../docs/local-development.md).

## Prerequisites

- `uv sync` in the repository root. CI installs only the deploy group:
  `uv run --only-group deploy`.
- A `package/` directory holding the Linux build. The Linux build runs only in CI, so download the
  `package` artifact of a Deploy run into `package/`, or run `uv run poe deploy package` on a
  machine that has one.
- `deploy/secrets/<id>/.env` for each server you deploy (see [Secrets](#secrets)).
- Docker hosts: a prepared host (see [Docker hosts](#docker-hosts)) and an SSH key.
- Panel servers: an API key from the panel's Account -> API Credentials page.

## Inventory

[`inventory.yml`](inventory.yml) holds everything that is not secret. An unknown key fails the
load, and so do two servers with one id, two instances of a server with one name or port, and a
server using a plugin that is not declared under `plugins`.

```yaml
runtime_image: ghcr.io/voltygg/cs2-plugins/cs2-server-runtime

database:
  sslMode: require

plugins:
  admin-system:
    database: admin_system
  bhop: {}

servers:
  - id: panel-a
    kind: panel
    environment: prod-panel-a
    panel_url: https://panel.example.com/server/abc12345
    host: 203.0.113.20
    plugins: [admin-system, bhop]
    instances:
      - { name: main, port: 27015, hostname: "CS2 Main" }

  - id: box-a
    kind: docker
    enabled: false
    environment: prod-box-a
    host: 203.0.113.10
    plugins: [admin-system]
    instances:
      - { name: main, port: 27015, map: de_dust2, hostname: "CS2 Main" }
      - { name: retake, port: 27016, map: de_mirage, plugins: [admin-system, bhop] }
```

| Key | Where | Default | Meaning |
| --- | --- | --- | --- |
| `runtime_image` | top level | required | Docker image repository, without a tag; each deploy picks the tag |
| `database` | top level | `port: 5432`, `user: postgres`, `sslMode: prefer` | connection settings every database-backed plugin shares; `host` comes from `DB_HOST` when unset |
| `plugins.<name>.database` | plugin | none | the database name this plugin connects to |
| `plugins.<name>.settings` | plugin | `{}` | what production changes in that plugin's settings |
| `id`, `kind`, `host` | server | required | `kind` is `panel` or `docker` |
| `environment` | server | none | the GitHub Environment holding this server's secrets |
| `enabled` | server | `true` | `false` skips it when no `--server` is given |
| `plugins` | server | `[]` | which plugins its instances run |
| `ssh_user`, `ssh_port` | docker | `steam`, `22` | |
| `cs2_root`, `deploy_root` | docker | `/home/steam/cs2`, `/home/steam/cs2/deploy` | |
| `panel_url` | panel | required | the server's panel page, `https://<panel>/server/<id>` |
| `game_dir` | panel | `/game/csgo` | |
| `name`, `port` | instance | required | |
| `hostname` | instance | `CS2 <name>` | |
| `map` | instance | `de_dust2` | |
| `insecure` | instance | `false` | Docker hosts: start with `-insecure` |
| `plugins` | instance | the server's list | replaces it; it does not extend it |

A panel server takes exactly one instance.

### Plugin settings

A deployed `settings.jsonc` starts from the plugin's own `plugins/<name>/configs/settings.jsonc`,
and the inventory lists only what production changes:

```yaml
plugins:
  bhop:
    settings:
      plugin: { locale: ru }
      bhop: { mode: grants }
```

`settings` is deep-merged into the plugin file; a list or scalar replaces what it lands on. A key
the plugin's `settings.jsonc` does not set fails the deploy. `${NAME}` in a string comes
from the server's secrets, plus `${SERVER_TAG}` (`<server-id>-<instance-name>`) and
`${SERVER_NAME}` (the instance hostname); an unset name fails the deploy. Per-server admin grants
reference the tag, so server ids and instance names have to stay stable.

A plugin with a `database` gets its `database` section from the inventory's `database` block plus
`DB_HOST` and `DB_PASSWORD`.

## Secrets

Each server reads `deploy/secrets/<id>/.env`. A variable already set in the process fills a
key the file lacks, and `SSH_KEY_FILE` from the process wins outright so CI can use its own key.
The repository's root `.env` is never read.

```bash
cp deploy/secrets/panel-a/.env.example deploy/secrets/panel-a/.env
```

| Key | Used for |
| --- | --- |
| `DB_HOST`, `DB_PASSWORD` | plugins with a database |
| `RCON_PASSWORD` | `poe rcon`; Docker instances also start with it |
| `CHEAT_API_KEY` | admin-system's `${CHEAT_API_KEY}` |
| `PANEL_API_KEY` | panel servers |
| `SSH_KEY_FILE` | Docker hosts: the private key path |
| `GSLT_<instance>` | Docker instances; empty starts in LAN mode |

## What a deploy ships

`AddonsBuilder` rebuilds the instance's `addons` tree from `package/` on every deploy: the voltmod
host, then each plugin the instance runs, then that plugin's rendered `settings.jsonc`.

```text
addons/
  metamod/voltmod.vdf             the only Metamod manifest
  voltmod/
    bin/linuxsteamrt64/            the host binary
    gamedata/                      shipped once, with the host
    plugins/<plugin>/
      plugin.json                  name, version and dependencies
      <plugin>.so                  the plugin module
      configs/settings.jsonc       rendered per instance
```

Metamod loads the host and nothing else. The host reads each
`addons/voltmod/plugins/<plugin>/plugin.json` and
loads the plugins itself; a plugin has no Metamod manifest of its own.

The host and the plugins are one ABI: the host refuses a plugin built against a different
`HostAbiVersion`. So `deploy package` always stages the host, the builder always puts it in the
payload, and a payload with no staged host fails the deploy rather than dropping plugins onto
whatever host the server already runs.

A deploy also removes the inventory plugins an instance no longer runs. Plugins outside the
inventory are left alone.

## Panel servers

Map, GSLT, hostname and RCON password live in the panel's Startup tab, not the inventory.

`deploy push` builds the addons tree, reads `gameinfo.gi` and the installed Metamod build, then:
downloads Metamod when the mirror has a newer one, writes the patched `gameinfo.gi`, stops the
server (overwriting a loaded `.so` can crash it), installs Metamod, uploads the tree, removes
unassigned plugins, and starts the server again. A failure after the stop starts the server before
reporting the error. `--dry-run` only reads.

It does not check that the plugins loaded - run `uv run poe rcon "volt list"`. `deploy restart` just
stops and starts the server, so the egg runs its own SteamCMD update.

Some hosts link every server to one shared CS2 install and refuse to read those links. There the
deploy replaces `gameinfo.gi` with a patched copy of [`panel/gameinfo.gi`](panel/gameinfo.gi),
which does not follow CS2 updates: when an update changes `gameinfo.gi`, copy it from an updated
dedicated server into that file and deploy again.

Turn off the egg's validate-on-start option if it has one. Validation restores `gameinfo.gi` and
Metamod stops loading.

## Docker hosts

Prepare a fresh Ubuntu 24.04+ machine:

```bash
sudo bash deploy/docker/bootstrap-host.sh     # --skip-docker if Docker is installed
```

It creates the `steam` user and directories, installs Docker with Compose, and opens SSH and UDP
ports 27015-27035.

```text
/home/steam/cs2/server                      shared SteamCMD install
/home/steam/cs2/deploy/docker-compose.yml   one instance; a copy of docker/docker-compose.yml
/home/steam/cs2/deploy/instances/<name>/    addons, .env, pre.sh, plugin bundles
```

[`docker/docker-compose.yml`](docker/docker-compose.yml) describes a single instance and is never
generated. Each instance runs as its own Compose project, `cs2-<name>`, and its `.env` fills in
the image, container name, port and paths. On the host:

```bash
cd /home/steam/cs2/deploy
docker compose -p cs2-main --env-file instances/main/.env ps    # or logs, restart, down
```

The shared install is mounted into every container, and each instance's own `addons` directory is
mounted over `csgo/addons`, so instances on one host can run different plugins.

Before CS2 starts, [`docker/pre.sh`](docker/pre.sh) copies the instance's bundle into place,
patches `gameinfo.gi`, and reinstalls Metamod when the mirror has a newer build. Set `MMS_URL` to
pin a build or `MMS_BASE` to use another mirror. It stops the container when the bundle has no
`voltmod.vdf`, since nothing the deploy shipped would load.

`deploy push` renders the tree under `deploy/.render/<id>`, creates the bind-mount directories,
syncs to `deploy_root` (the sync never deletes, so the installed Metamod survives), removes the
plugin folders an instance no longer uses, pulls the runtime image at `RUNTIME_IMAGE_TAG` (the
commit SHA in CI, `latest` otherwise), recreates the instances one at a time so `pre.sh` reinstalls
them, checks that each is running, and removes the image's other tags. `--dry-run` renders,
previews the sync and prints the remote commands.

SteamCMD only runs when a container starts, so a running server never gets a Valve update.
`deploy restart` restarts the instances one at a time and waits for SteamCMD in between.

Reach the database through an SSH tunnel:

```bash
uv run poe deploy tunnel --server box-a     # --local-port, --db-host, --db-port
psql "host=127.0.0.1 port=5433 dbname=admin_system user=postgres"
```

Ctrl-C closes it. `poe rcon` opens the same kind of tunnel on its own, since the host firewall
leaves only the game's UDP port open.

`deploy cleanup box-a --yes` removes the containers, every tag of the runtime image,
`deploy_root` and `cs2_root`. Docker, PostgreSQL, firewall rules, the `steam` user, registry
packages and plugin databases stay. `--dry-run` previews it.

## Databases

The deploy does not create databases. Before the first deploy, create one per database-backed
plugin, named as in the inventory. Plugins connect as `postgres` on port 5432 unless the
inventory's `database` block says otherwise, and apply their own migrations when they load.

## CI

Push to `prod`, or run the Deploy workflow by hand with an optional server and dry run. It builds
and tests the Linux plugins, packages the host and every inventory plugin, builds and pushes the
runtime image when a Docker host is involved and it is not a dry run, and deploys each server in
its own job under that server's GitHub Environment.

Each GitHub Environment holds `SERVER_ENV` (the whole `.env` file) and, for a Docker host,
`SSH_KEY`. CircleCI only builds and tests.

The voltmod toolchain is pinned in `conan.lock`. After relocking to a new voltmod release, bump the
tag in `.github/workflows/ci.yml`, `.github/workflows/deploy.yml` and `.circleci/config.yml`.

## Layout

```text
deploy/
  inventory.yml               servers, instances, plugins and their settings
  secrets/<id>/.env           a server's secrets (gitignored; copy .env.example)
  docker/                     runtime image, docker-compose.yml, pre-launch hook, host setup script
  panel/gameinfo.gi           gameinfo.gi for panel hosts with a linked CS2 install
  tools/                      the CLI, one class per file
    cli.py                the commands
    deployer.py           Deployer: what every server kind implements
    deployer_factory.py   DeployerFactory: builds the Deployer for each server's kind
    paths.py, errors.py, rcon.py, console.py
    config/               servers.py (models), inventory.py, secrets.py
    addons/               packager.py, settings.py, builder.py
    panel/                api.py, metamod.py, gameinfo.py, deployer.py
    docker/               ssh.py, tunnel.py, host.py, compose.py, deployer.py
```

A new server kind is a model in `config/servers.py`, a package with a `Deployer` subclass, and one
entry in `DeployerFactory.BY_KIND`.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| `no package for <name>` | `package/` is empty; download the CI artifact or run `deploy package` |
| `package/host has no metamod/voltmod.vdf` | the package set has no host; rebuild the framework and repackage |
| The server starts but no plugin loads | `uv run poe rcon "meta list"` for the host, then `"volt list"` for the plugins |
| A plugin loads with stale settings | it is not in the instance's `plugins` list, so its `settings.jsonc` was never rendered |
| Metamod stops loading after a CS2 update | `gameinfo.gi` was restored; redeploy, and on a panel turn off validate-on-start |
| A plugin from an old install keeps loading | delete its leftover `addons/metamod/<plugin>.vdf` by hand; the tooling only manages `voltmod.vdf` |
