# Contributing to CS2 plugins

Plugin code lives under `plugins/<name>/`. VoltMod supplies the shared engine
layer through Conan.

## Set up

Install Python 3.14+, uv, and a C++23 compiler:

```powershell
git clone https://github.com/voltygg/cs2-plugins.git
cd cs2-plugins
uv sync
uv run poe doctor
uv run poe bootstrap
```

Bootstrap installs the Conan configuration, builds, and runs tests. See
[Create your first plugin](docs/getting-started-plugin.md) to run a plugin on a
local server.

## Add a plugin

```powershell
uv run poe new-plugin fun-votes
```

The command creates `plugins/fun-votes` and registers it in the root
`CMakeLists.txt`. `voltmod_add_plugin` discovers `.cpp` files below `src/`.

For manual setup, create the plugin CMake file with
`voltmod_add_plugin(<name> VERSION <version>)` and add its
`add_subdirectory` line to the root project.

Third-party C++ dependencies require:

1. A Conan requirement in `conanfile.py`.
2. `find_package` in the root `CMakeLists.txt`.
3. The imported target in the plugin's CMake libraries.

## Code rules

- Use C++23 and `.hpp` headers.
- Use `PascalCase` for types and methods, `_camelCase` for members, and
  `camelCase` for local values and parameters.
- Prefer `std::format`, designated initializers, and `int64_t` SteamIDs.
- Keep game code on the main thread. VoltMod returns database and HTTP
  completions to that thread.
- Pass services through constructors; do not add singletons.
- Keep files near 300 to 350 lines when that improves readability.
- Comment non-obvious contracts, reasons, and constraints instead of syntax.

## Change VoltMod with a plugin

`vendor/voltmod` is a separate Git repository. Point Conan at it temporarily:

```powershell
conan editable add vendor/voltmod
uv run poe build
conan editable remove voltmod
```

Inspect, validate, and commit the two repositories independently. Without an
editable package, Conan resolves `voltmod/[~1.2]` from the public remote.

## Before pushing

```powershell
uv run poe lint
uv run poe format
uv run poe test
```

`build` compiles only, so the local loop stays fast; `test` is what runs the
suite.

GitHub Actions is the primary CI and deployment path. CircleCI is the fallback;
do not let both deploy `prod` unless duplicate deployment is intentional.
