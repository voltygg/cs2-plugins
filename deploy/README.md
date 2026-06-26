# Deploy

Provisioning + deployment for hosting this monorepo's plugins on Ubuntu servers.
One shared PostgreSQL instance (a database **per plugin**), and a fleet of game
boxes that each run a chosen **subset** of plugins across one or more CS2 instances.

`scripts/deploy.sh` / `scripts/start-server.sh` remain the **local Windows dev**
tools and are unaffected by anything here.

## How it fits together

```text
inventory.yml ── the source of truth: servers × plugins × instances, shared DB
   │
   ├─ provision/   one-time, run ON a fresh VPS as root (SteamCMD, CS2, Metamod,
   │               steam user, firewall, systemd unit, maintenance timers)
   ├─ systemd/     the cs2-server@.service template + launch/update/backup wrappers
   ├─ config/      per-plugin settings templates + per-server values/secrets (SOPS)
   └─ bin/         inventory.py, package-plugin.sh, render-config.sh, ensure-databases.sh,
                   deploy-remote.sh
```

Build (Linux `.so`) and deploy are wired into `.github/workflows/deploy.yml`,
triggered by pushing to the **`prod`** branch.

## Prerequisites (operator machine)

- `python3` + PyYAML (`pip install pyyaml` or `uv sync`), `rsync`, `ssh`, `gettext` (`envsubst`).
- `sops` + `age` for secrets (only if you deploy/render locally).

## 1. One-time: provision a game box

Copy the repo (or just `deploy/`) to the box and run:

```bash
sudo bash deploy/provision/00-bootstrap.sh
```

This installs SteamCMD + CS2 (app 730), Metamod:Source 2.0 (patching `gameinfo.gi`),
creates the `steam` user, opens the firewall (SSH + `27015:27035`), installs the
`cs2-server@.service` template, grants the deploy user passwordless `systemctl` over
the `cs2-server@` instances, and enables the update/backup timers. Re-running is safe.

Useful overrides (env vars): `MMS_URL` (pin a Metamod build), `CS2_PORT_RANGE`,
`SSH_PORT`. Instances are **not** started here — `deploy-remote.sh` does that once it
has pushed their env files.

## 2. One-time: the shared database

On (or with network access to) your PostgreSQL host, create a database per plugin:

```bash
APP_DB_PASSWORD='<app-role-pw>' PGPASSWORD='<superuser-pw>' \
  bash deploy/bin/ensure-databases.sh --admin-user postgres
```

`APP_DB_PASSWORD` must match the `DB_PASSWORD` in each server's secrets. Plugins apply
their own migrations on load (the Migrator), so no schema is created here.

## 3. Secrets (SOPS + age)

```bash
age-keygen -o age.key                       # do this ONCE; keep the private key safe
# put the PUBLIC key (age1...) into deploy/.sops.yaml
```

Per server, create and encrypt its unified env (non-secret values + secrets in one file):

```bash
cd deploy/config/servers/box-a
cp .env.example .env          # fill DB_PASSWORD, CHEAT_*, per-instance GSLT/RCON, ...
sops -e .env > .sops.env      # encrypted; commit THIS
rm .env                       # never commit the plaintext
```

In CI, each server's GitHub **Environment** holds `SOPS_AGE_KEY` (the age *private*
key) and `SSH_PRIVATE_KEY` (a deploy key for that box).

## 4. Deploy

### Via CI (normal path)

Push to `prod`. The workflow builds Linux bundles, then fans out a deploy job per
server (gated by its Environment). Each job pushes only that server's plugins,
renders its `settings.jsonc`, writes the instance env files, and restarts + verifies
the `cs2-server@` units. Use **Actions → Deploy → Run workflow** for a manual run
(optional single-server filter and dry-run).

### Manually (emergency / first bring-up)

```bash
docker compose run --rm build                 # produces objdir/.../linux-x86_64/*.so
bash deploy/bin/package-plugin.sh admin-system linux
SOPS_AGE_KEY="$(cat age.key)" \
  bash deploy/bin/deploy-remote.sh --server box-a
```

Add `--dry-run` to preview, or `--preserve-settings` to keep the box's existing
`settings.jsonc` instead of rendering it.

## Common changes

- **Add a server:** add an entry under `servers:` in `inventory.yml`, create
  `config/servers/<id>/.sops.env` (from `.env.example`), create a GitHub Environment
  `<environment>` with its `SSH_PRIVATE_KEY` + `SOPS_AGE_KEY`, then provision the box.
- **Give a server a different plugin set:** edit that server's `plugins:` list.
- **Add a plugin:** create `plugins/<name>/` as usual, declare it under `plugins:` in
  `inventory.yml` with its own `database:`, add `config/plugins/<name>/settings.template.jsonc`,
  re-run `ensure-databases.sh`, and add `<name>` to the servers that should run it.
- **Change config:** edit the per-plugin template (non-secret) or a server's
  `.sops.env`, then redeploy — the rendered `settings.jsonc` is
  overwritten on the box (config-as-code). Set `--preserve-settings` for boxes you
  hand-edit.

## The win64 → linuxsteamrt64 VDF

The committed `plugins/<name>/<name>.vdf` points at `bin/win64` (Windows dev).
`package-plugin.sh` **generates** the Linux VDF (`bin/linuxsteamrt64`) into each
bundle, so the server gets the correct one automatically.
