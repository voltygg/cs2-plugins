# Deployment Scripts

Deployment scripts copy built Metamod:Source plugins to a CS2 server and also
deploy the shared cs2-kit gamedata.

## Usage

```bash
./scripts/deploy.sh
./scripts/deploy.sh --server-path "D:/CS2-Server"
./scripts/deploy.sh --plugin-name admin-system
./scripts/deploy.sh --server-path "D:/CS2-Server" --plugin-name admin-system
```

On Windows, run from Git Bash or WSL. The default Windows build preset is
`windows-msvc-release`; override it with `CS2_BUILD_PRESET`.

```bash
CS2_BUILD_PRESET=windows-msvc-debug ./scripts/deploy.sh --plugin-name admin-system
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
  cs2-kit/
    gamedata/
```

Linux package bundles use:

```text
addons/<plugin>/bin/linuxsteamrt64/<plugin>.so
```

## Adding Another Plugin

1. Create `plugins/<new>/src/`, `plugins/<new>/CMakeLists.txt`, configs, and
   `plugins/<new>/<new>.vdf`.
2. Call `cs2_add_plugin(<new> ...)` in that plugin CMake file.
3. Add `add_subdirectory(plugins/<new>)` to the root `CMakeLists.txt`.
4. Add any new third-party C++ deps to `conanfile.py` and
   `cmake/ThirdParty.cmake`.

## Notes

- Existing `settings.jsonc` files are preserved to avoid overwriting DB
  credentials or per-server config.
- Debug symbols are copied if they exist.
- Empty config directories are skipped.
- `deploy/tools/cli.py package` generates platform-correct Metamod VDF files
  for Docker deployment bundles.
