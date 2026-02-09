# Deployment Scripts

Generic deployment scripts for Metamod:Source 2.0 plugins that auto-detect plugin name from `AMBuildScript`.

## Features

- **Auto-Detection**: Automatically detects plugin name from `PLUGIN_NAME` in `AMBuildScript`
- **Cross-Platform**: Works on both Windows and Linux
- **Flexible**: Can override plugin name and architecture
- **Smart Copying**: Only copies directories that exist (configs, assets, gamedata)
- **Safe**: Preserves existing `database.json` to prevent overwriting production configs

## Usage

### PowerShell (Windows)

```powershell
# Basic usage (auto-detect plugin, default server path)
.\scripts\deploy.ps1

# Custom server path
.\scripts\deploy.ps1 -ServerPath "D:\CS2-Server"

# Override plugin name
.\scripts\deploy.ps1 -PluginName "my-custom-plugin"

# Specify architecture
.\scripts\deploy.ps1 -Architecture "x86"

# All options combined
.\scripts\deploy.ps1 -ServerPath "D:\CS2-Server" -PluginName "my-plugin" -Architecture "x86_64"
```

### Bash (Linux)

```bash
# Basic usage (auto-detect plugin, default server path)
./scripts/deploy.sh

# Custom server path
./scripts/deploy.sh /opt/cs2-server

# Override plugin name
./scripts/deploy.sh --plugin-name my-custom-plugin

# Specify architecture
./scripts/deploy.sh --arch x86

# All options combined
./scripts/deploy.sh --plugin-name my-plugin --arch x86_64 /opt/cs2-server

# Get help
./scripts/deploy.sh --help
```

## How Plugin Name is Detected

1. **Explicit parameter**: If `--plugin-name` / `-PluginName` is provided, use it
2. **AMBuildScript**: Extract from `PLUGIN_NAME = "plugin-name"` in `AMBuildScript`
3. **Directory name**: Fallback to current directory name

## Expected Project Structure

```
your-plugin/
├── AMBuildScript              # Contains PLUGIN_NAME = "your-plugin"
├── your-plugin.vdf            # Metamod plugin registration (optional)
├── configs/                   # Configuration files (optional)
│   ├── config.json
│   ├── database.json
│   └── ...
├── gamedata/                  # Signatures/offsets (optional)
│   └── plugin.games.txt
└── objdir/
    └── src/
        └── your-plugin/
            ├── windows-x86_64/
            │   ├── your-plugin.dll
            │   └── your-plugin.pdb
            └── linux-x86_64/
                ├── your-plugin.so
                └── your-plugin.so.dbg
```

## Deployment Target Structure

Files are deployed to the following structure in your CS2 server:

```
csgo/
└── addons/
    ├── metamod/
    │   ├── your-plugin.dll (or .so)
    │   ├── your-plugin.vdf
    │   └── your-plugin.pdb (or .so.dbg)
    └── your-plugin/
        ├── configs/
        └── gamedata/
```

## Reusing for Other Plugins

These scripts are completely generic! To use with another plugin:

1. Copy `scripts/deploy.ps1` and `scripts/deploy.sh` to your new plugin project
2. Ensure your `AMBuildScript` has `PLUGIN_NAME = "your-plugin-name"`
3. Create a `.vdf` file named `your-plugin-name.vdf`
4. Run the deployment script

Example `.vdf` file:

```vdf
"Plugin"
{
    "file"  "../metamod/your-plugin-name"
}
```

## Notes

- The script preserves existing `database.json` to prevent overwriting production database credentials
- Debug symbols (.pdb/.so.dbg) are automatically copied if they exist
- Empty directories (like assets with only `.gitkeep`) are skipped
- The script validates the server path before deploying
