# Deployment

The deploy CLI ships the plugins to two kinds of server:

- **Panel servers** (`kind: panel`) run on a Pterodactyl panel. The CLI uploads
  files and restarts the server through the panel's client API, with no SSH or
  Docker.
- **Docker hosts** (`kind: docker`) are Linux machines you control. Each CS2
  instance runs in its own container built from `joedwards32/cs2`, and the
  instances on one host share a SteamCMD install. The CLI drives the host over SSH.

`uv run poe install` and `uv run poe start-server` cover a local server. This guide
covers remote ones.

## Layout

```text
deploy/
  inventory.yml               servers, instances, plugins and their settings
  secrets/servers/<id>/.env   a server's secrets (gitignored; copy .env.example)
  docker/                     runtime image, pre-launch hook, host setup script
  panel/gameinfo.gi           gameinfo.gi for panel hosts with a linked CS2 install
  tools/                      the CLI
```

The CLI is a package with one class per file:

```text
deploy/tools/
  cli.py                the commands
  deployer.py           Deployer: what every server kind implements
  deployer_factory.py   DeployerFactory: creates the Deployer for each server's kind
  errors.py             DeployError
  paths.py              repository locations
  rcon.py               RconClient
  config/               servers.py (server models), inventory.py (Inventory), server_env.py (ServerEnv)
  addons/               packager.py (PluginPackager), settings.py (SettingsRenderer), builder.py (AddonsBuilder)
  panel/                api.py (PanelApi), metamod.py (Metamod), gameinfo.py (GameInfo), deployer.py
  docker/               ssh.py (Ssh), tunnel.py (Tunnel), host.py (DockerHost), compose.py (ComposeProject), deployer.py
```

A new server kind is a model in `config/servers.py`, a package with a `Deployer`
subclass, and one entry in `DeployerFactory.BY_KIND`.

## Commands

```bash
uv run poe deploy-package                             # stage the Linux build under package/
uv run poe deploy-server [--server ID] [--dry-run]    # install plugins, restart
uv run poe deploy-update [--server ID] [--dry-run]    # restart so CS2 updates
uv run poe rcon "meta list" [--server ID] [--instance NAME]
uv run poe deploy-tunnel --server ID                  # Docker hosts: database on 127.0.0.1:5433
uv run poe deploy-cleanup --server ID --yes           # Docker hosts: remove the deployment
```

Without `--server`, `deploy-server` and `deploy-update` act on every enabled server;
`rcon` and `deploy-tunnel` pick the only enabled server.

The Linux build runs only in CI. To deploy from your machine, download the
`package` artifact of a Deploy run into `package/`.

## Inventory

[`inventory.yml`](inventory.yml) holds everything that is not secret:

```yaml
runtime_image: ghcr.io/voltygg/cs2-plugins/cs2-server-runtime

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

- Every plugin a server uses must be listed under `plugins`; CI packages exactly
  that list.
- An instance's `plugins` replaces the server's list; it does not extend it.
- `environment` names the GitHub Environment that holds the server's secrets.
- `runtime_image` is a repository without a tag; see [Docker hosts](#docker-hosts).
- An unknown key fails the load, and so do two servers with one id or two
  instances of a server with one name or port.
- Docker hosts also take `ssh_user` (`steam`), `ssh_port` (`22`), `cs2_root`
  (`/home/steam/cs2`) and `deploy_root` (`/home/steam/cs2/deploy`). Panel servers
  take `game_dir` (`/game/csgo`) and exactly one instance.
- Instances take `map` (`de_dust2`) and, on Docker hosts, `insecure: true` to start
  with `-insecure`.

## Plugin settings

A deployed `settings.jsonc` starts from the plugin's own
`plugins/<name>/configs/settings.jsonc`. The inventory lists only what production
changes:

```yaml
plugins:
  bhop:
    settings:
      plugin: { locale: ru }
      bhop: { mode: grants }
```

- `settings` is deep-merged into the plugin file; a list replaces the whole list.
- A key the plugin's `settings.schema.json` does not define fails the deploy.
- `${NAME}` in a string comes from the server's secrets, plus `${SERVER_TAG}`
  (`<server-id>-<instance-name>`) and `${SERVER_NAME}` (the instance hostname).
  Per-server admin grants reference the tag, so server ids and instance names must
  stay stable.
- A plugin with a `database` gets its `database` section from the inventory's
  `database` block, `DB_HOST` and `DB_PASSWORD`.

## Secrets

Each server reads its secrets from `deploy/secrets/servers/<id>/.env`:

```bash
cp deploy/secrets/servers/panel-a/.env.example deploy/secrets/servers/panel-a/.env
```

| Key | Used for |
| --- | --- |
| `DB_HOST`, `DB_PASSWORD` | plugins with a database |
| `RCON_PASSWORD` | `poe rcon`; Docker instances also start with it |
| `CHEAT_API_KEY` | admin-system's `${CHEAT_API_KEY}` |
| `PANEL_API_KEY` | panel servers (panel → Account → API Credentials) |
| `SSH_KEY_FILE` | Docker hosts: the private key path |
| `GSLT_<instance>` | Docker instances; empty starts in LAN mode |

A variable set in the process fills a key the file lacks. `SSH_KEY_FILE` from the
process wins over the file, so CI can use its own key. The repository's root `.env`
is never read.

## Databases

The deploy does not create databases. Before the first deploy, create one per
database-backed plugin, named as in the inventory. Plugins connect as `postgres` on
port 5432 unless the inventory's `database` block sets `user` or `port`, and apply
their own migrations when they load.

## CI

Push to `prod`, or run the Deploy workflow by hand with an optional server and dry
run. The workflow:

1. Builds and tests the Linux plugins, then packages every inventory plugin.
2. When the deploy includes a Docker host and is not a dry run, builds the runtime
   image and pushes it tagged with the commit SHA and `latest`.
3. Deploys each server in its own job, with that server's GitHub Environment.

Each GitHub Environment holds `SERVER_ENV` (the whole `.env` file) and, for a
Docker host, `SSH_KEY` (the private key). CircleCI only builds and tests; it does not
deploy.

The voltmod toolchain is pinned to the release in `conan.lock` (now `v1.4.6`). After
relocking to a new voltmod release, bump the tag in `.github/workflows/ci.yml`,
`.github/workflows/deploy.yml` and `.circleci/config.yml`.

## Panel servers

A panel server's map, GSLT, hostname and RCON password live in the panel's Startup
tab, not the inventory. `deploy-server`:

1. Builds the addons tree and patches `gameinfo.gi` to load Metamod.
2. Downloads Metamod when the mirror has a newer build.
3. Stops the server, since overwriting a loaded plugin can crash it.
4. Installs Metamod, uploads the plugins and removes inventory plugins the server no
   longer uses. If a step fails, it starts the server again before reporting the
   error.
5. Starts the server and waits until the panel reports it running.

It does not check that the plugins loaded; run `uv run poe rcon "meta list"`.
`--dry-run` only reads from the panel. `deploy-update` restarts the server so the
egg runs its SteamCMD update.

Some hosts link every server to one shared CS2 install and refuse to read those
links. There the deploy replaces `gameinfo.gi` with a patched copy of
[`panel/gameinfo.gi`](panel/gameinfo.gi), which does not follow CS2 updates. When an
update changes `gameinfo.gi`, copy it from an updated dedicated server into that
file and deploy again.

Turn off the egg's validate-on-start option if it has one: validation restores
`gameinfo.gi`, and Metamod stops loading.

## Docker hosts

### Prepare a host

On a fresh Ubuntu 24.04+ server:

```bash
sudo bash deploy/docker/bootstrap-host.sh                 # --skip-docker if Docker is installed
```

It creates the `steam` user and directories, installs Docker with Compose, and
opens SSH and UDP ports 27015-27035.

### What runs on the host

```text
/home/steam/cs2/server                      shared SteamCMD install
/home/steam/cs2/deploy/docker-compose.yml   one service per instance
/home/steam/cs2/deploy/instances/<name>/    addons, .env, pre.sh, plugin bundles
```

The shared install is mounted into each container, and the instance's own `addons`
directory is mounted over `csgo/addons`, so instances on one host can run different
plugins.

Before CS2 starts, [`docker/pre.sh`](docker/pre.sh) copies the instance's plugins
into place, patches `gameinfo.gi`, and installs Metamod when the mirror has a newer
build. Set `MMS_URL` to pin a build or `MMS_BASE` to use another mirror.

### Deploy

`deploy-server`:

1. Renders the tree under `deploy/.render/<id>` and syncs it to `deploy_root`. The
   sync never deletes files, so the installed Metamod survives.
2. Deletes the inventory plugins an instance no longer uses.
3. Pulls the runtime image tagged `RUNTIME_IMAGE_TAG`: the commit SHA in CI,
   `latest` otherwise.
4. Recreates the instances one at a time, so `pre.sh` installs the new plugins, and
   waits up to 90 minutes for each instance's SteamCMD run.
5. Checks that every instance runs, then removes the image's other tags.

`--dry-run` renders, previews the sync and prints the remote commands without
running them.

### Update CS2

SteamCMD runs when a container starts, so a running server gets no Valve update.
`deploy-update` restarts the instances one at a time and waits for SteamCMD in
between.

### Reach the database

```bash
uv run poe deploy-tunnel --server box-a     # --local-port, --db-host, --db-port
psql "host=127.0.0.1 port=5433 dbname=admin_system user=postgres"
```

Ctrl-C closes the tunnel. `poe rcon` opens the same kind of tunnel on its own, since
the firewall leaves only the game's UDP port open.

### Remove a deployment

```bash
uv run poe deploy-cleanup --server box-a --dry-run
uv run poe deploy-cleanup --server box-a --yes
```

This removes the containers, every tag of the runtime image, `deploy_root` and
`cs2_root`. Docker, PostgreSQL, firewall rules, the `steam` user, registry packages
and plugin databases stay.
