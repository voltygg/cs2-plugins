# Contributing to CS2 plugins

Plugin code lives under `plugins/<name>/`. VoltMod supplies the shared engine
layer through Conan.

## Set up and build

Install the project tools and VoltMod's Conan configuration:

```bash
uv sync
uv run poe bootstrap
```

`bootstrap` builds once. Later builds can use:

```bash
uv run poe build
uv run poe build windows-msvc-debug
uv run poe build-linux
```

The supported presets are `windows-msvc-release`, `windows-msvc-debug`,
`linux-steamrt-release`, and `linux-steamrt-debug`. Output is written to
`build/<preset>/plugins/<name>/<platform-arch>/`.

Windows builds require MSVC; Linux builds use GCC 14 in Valve's Steam Runtime.
The `voltmod` Python distribution pins CMake, Conan, Ninja, and clang-format.

## Add code or a plugin

`voltmod_add_plugin` discovers `.cpp` files below a plugin's `src/` directory.
Add a source file there and rebuild.

To create a plugin, run:

```bash
uv run poe new-plugin <name>
```

For a manual setup, add `plugins/<name>/CMakeLists.txt` with
`voltmod_add_plugin(<name> VERSION <version> ...)` and register it with
`add_subdirectory(plugins/<name>)` in the root `CMakeLists.txt`.

Add a third-party C++ dependency in three places:

1. Add its Conan requirement to `conanfile.py`.
2. Call `find_package` in the root `CMakeLists.txt`.
3. Link the imported target from the plugin's CMake file.

## Code rules

- Use C++23 and `.hpp` headers.
- Use `PascalCase` for types and methods, `_camelCase` for members, and
  `camelCase` for local variables and parameters.
- Use `std::format`, designated initializers, and `int64_t` SteamIDs.
- Keep game code on the main thread. VoltMod owns the database and HTTP worker
  services and returns their callbacks to the game thread.
- Pass services through constructors; do not add singletons.
- Keep files near 300-350 lines when practical.
- Add comments only for non-obvious reasons, contracts, or constraints.

## Change VoltMod with a plugin

`vendor/voltmod` is a separate Git worktree. Point Conan at it while developing
both repositories:

```bash
conan editable add vendor/voltmod
uv run poe build
conan editable remove voltmod
```

Any VoltMod checkout can be used in place of `vendor/voltmod`. Without an
editable entry, Conan resolves the `voltmod/[~1.2]` requirement from the remote.
Land and release the framework change before updating a consumer that needs it.

## Before pushing

```bash
uv run poe lint
uv run poe format-check
uv run poe build
ctest --preset <preset>
```

CI runs the same Ruff, clang-format, Linux build, and CTest checks.

GitHub Actions is the primary CI and deployment path. CircleCI mirrors it as a
fallback. Do not leave both providers deploying the `prod` branch unless two
deployments are intentional. See [.circleci/README.md](.circleci/README.md) for
CircleCI setup.
