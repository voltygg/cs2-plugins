# Deployment Scripts

Deployment script for the Metamod:Source 2.0 plugins in this monorepo. It deploys every plugin under `plugins/` (or a single one via `--plugin-name`) to a CS2 server, plus the shared cs2-kit gamedata.

## Features

- **Multi-plugin**: Deploys every plugin under `plugins/` by default, or a single one with `--plugin-name`
- **Cross-Platform**: Works on both Windows and Linux
- **Flexible**: Can target a single plugin and override architecture
- **Smart Copying**: Only copies directories that exist (configs, translations, gamedata)
- **Safe**: Preserves an existing `settings.jsonc` on the server (holds DB creds + per-server config)

## Usage

```bash
# Basic usage (all plugins under plugins/, default server path)
./scripts/deploy.sh

# Custom server path
./scripts/deploy.sh --server-path "D:/CS2-Server"

# Deploy a single plugin
./scripts/deploy.sh --plugin-name "admin-system"

# Specify architecture
./scripts/deploy.sh --architecture x86

# All options combined
./scripts/deploy.sh --server-path "D:/CS2-Server" --plugin-name "admin-system" --architecture x86_64

# Get help
./scripts/deploy.sh --help
```

On Windows, run from Git Bash or WSL. On Linux, run directly.

## Which Plugins Are Deployed

1. **`--plugin-name`**: If provided, deploy only that plugin from `plugins/<name>/` (errors if it has no built binary).
2. **All plugins**: Otherwise, deploy every directory under `plugins/`. A plugin with no built binary yet is skipped with a notice.

## Expected Project Structure

```text
admin-system/                       # monorepo root
├── AMBuildScript                   # discovers plugins/*/src/AMBuilder, builds one binary each
├── vcpkg.json                      # root C++ deps (union across all plugins)
├── plugins/
│   └── admin-system/
│       ├── admin-system.vdf        # Metamod plugin registration
│       ├── configs/                # settings.jsonc + migrations/ + translations/
│       └── src/
│           └── AMBuilder           # this plugin's sources + link libs
├── vendor/cs2-kit/gamedata/        # shared signatures/offsets (deployed once)
└── objdir/
    └── plugins/
        └── admin-system/
            └── src/
                └── admin-system/
                    ├── windows-x86_64/
                    │   ├── admin-system.dll
                    │   └── admin-system.pdb
                    └── linux-x86_64/
                        ├── admin-system.so
                        └── admin-system.so.dbg
```

## Deployment Target Structure

Files are deployed to the following structure in your CS2 server:

```text
csgo/
└── addons/
    ├── metamod/
    │   └── <plugin>.vdf
    ├── <plugin>/
    │   ├── bin/win64/<plugin>.dll (+ .pdb)
    │   └── configs/
    └── cs2-kit/
        └── gamedata/               # shared, copied once for all plugins
```

## Adding Another Plugin

1. Create `plugins/<new>/src/` with its own `AMBuilder` (copy admin-system's and adjust the link libs), plus `plugins/<new>/configs/` and `plugins/<new>/<new>.vdf`.
2. Append any new C++ deps to the root `vcpkg.json`.
3. `scripts/build.sh` builds it (the root `AMBuildScript` discovers it automatically), and `scripts/deploy.sh` ships it — no script edits needed.

Example `.vdf` file (`plugins/<new>/<new>.vdf`):

```vdf
"Metamod Plugin"
{
    "alias"  "<new>"
    "file"   "addons/<new>/bin/win64/<new>"
}
```

## Notes

- The script preserves an existing `settings.jsonc` on the server to avoid overwriting DB credentials / per-server config.
- Debug symbols (.pdb/.so.dbg) are automatically copied if they exist.
- Empty directories are skipped.
- The script validates the server path before deploying.
