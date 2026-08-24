# Deployment Scripts

Deployment scripts copy built Metamod:Source plugins to a CS2 server and also
deploy the shared voltmod gamedata.

## Usage

```bash
uv run poe deploy
uv run poe deploy --server-path "D:/CS2-Server"
uv run poe deploy --plugin-name admin-system
uv run poe deploy --server-path "D:/CS2-Server" --plugin-name admin-system
```

The default Windows build preset is `windows-msvc-release`; override it with
`CS2_BUILD_PRESET`.

```bash
CS2_BUILD_PRESET=windows-msvc-debug uv run poe deploy --plugin-name admin-system
```

## Build Output

The scripts expect binaries under:

```text
build/<preset>/plugins/<plugin>/<platform-arch>/
```

Examples:

```text
build/windows-msvc-release/plugins/admin-system/windows-x86_64/admin-system.dll
build/linux-steamrt-release/plugins/admin-system/linux-x86_64/admin-system.so
```

## Server Layout

Files are deployed under the CS2 server `csgo/` directory:

```text
addons/
  metamod/<plugin>.vdf
  <plugin>/
    bin/win64/<plugin>.dll
    configs/
  voltmod/
    gamedata/
```

Linux package bundles use:

```text
addons/<plugin>/bin/linuxsteamrt64/<plugin>.so
```

## Adding Another Plugin

1. Create `plugins/<new>/src/`, `plugins/<new>/CMakeLists.txt`, configs, and
   `plugins/<new>/<new>.vdf`.
2. Call `voltmod_add_plugin(<new> ...)` in that plugin CMake file.
3. Add `add_subdirectory(plugins/<new>)` to the root `CMakeLists.txt`.
4. Add any new third-party C++ deps to `conanfile.py`, `find_package` them in
   the root `CMakeLists.txt`, and link their imported targets in the plugin
   CMake file.

## Notes

- Existing `settings.jsonc` files are preserved to avoid overwriting DB
  credentials or per-server config.
- Debug symbols are copied if they exist.
- Empty config directories are skipped.
- `python -m deploy.tools.cli package` generates platform-correct Metamod VDF
  files for Docker deployment bundles.
