# Contributing to CS2 plugins

This repository contains C++23 Metamod:Source plugins for Counter-Strike 2.
[VoltMod](https://github.com/voltygg/voltmod) supplies the shared engine layer as
a Conan package. Plugin code lives under `plugins/<name>/`.

## Setup

Install the pinned tools, then install VoltMod's Conan profiles and remote:

```bash
uv sync
uv run poe bootstrap
```

On Windows, the build loads the MSVC environment through `vcvars64.bat`, so a
regular shell is sufficient.

Required tools:

- CMake 4.3.4 or newer
- Conan 2.29.1 or newer
- Ninja
- MSVC on Windows, GCC 14 on Linux (Steam Runtime sniper toolchain)

The CMake, Conan, Ninja, and clang-format pins are supplied by the `voltmod`
Python distribution listed in `pyproject.toml`.

## Build

```bash
uv run poe build                       # default preset for your platform
uv run poe build windows-msvc-debug    # explicit preset
uv run poe build-linux
```

Build output is written to:

```text
build/<preset>/plugins/<name>/<platform-arch>/
```

Available presets:

- `linux-steamrt-release`
- `linux-steamrt-debug`
- `windows-msvc-release`
- `windows-msvc-debug`

## Add code

The build discovers `.cpp` files under `plugins/<name>/src/`. Add a source file
there and rebuild.

To add a plugin, create `plugins/<new>/src/`, its configuration, and
`plugins/<new>/CMakeLists.txt` with
`voltmod_add_plugin(<new> VERSION <version> ...)`. Register it with
`add_subdirectory(plugins/<new>)` in the root `CMakeLists.txt`.

Add third-party C++ dependencies to `conanfile.py`, call `find_package` for them
in the root `CMakeLists.txt`, and link their imported targets (for example,
`libpqxx::pqxx`) in the plugin's `CMakeLists.txt`.

## Constraints

- C++23.
- Main-thread only. Metamod hooks run on the game thread. Do not add threads or
  mutexes in game code; synchronization belongs inside the framework's worker
  services.
- Use `.hpp` headers, never `.h`.
- Use C#-style naming: `PascalCase` types and methods, `_camelCase` members,
  `camelCase` locals and parameters, `PascalCase` constants, and `camelCase`
  JSON keys.
- Use `std::format`, designated initializers, and `int64_t` for SteamIDs.
- Use services and managers, not singletons. Use the runtime for VoltMod
  services and `App` for plugin managers.
- Keep source files around 300-350 lines when practical.
- Add comments only when they explain a non-obvious reason or constraint.

## Change VoltMod alongside a plugin

Point Conan at a local VoltMod checkout while changing both repositories:

`vendor/voltmod` is a submodule for this workflow
(`git submodule update --init` on an older clone), but any sibling checkout
works too.

```bash
conan editable add vendor/voltmod
uv run poe build          # pick up framework edits directly
conan editable remove voltmod
```

The editable entry points to a checkout, not a package version. Without it, the
build uses the Conan package selected by `conanfile.py`.

Land the framework change first in its own repository and pull request. A `v*` tag
publishes a new package. Update the range in `conanfile.py` only when the new
version falls outside `[~1]`.

## Before you push

- Run clang-format over C++ changes.
- Run `uv run poe lint` for Python tooling.
- Build with `uv run poe build`.
- Run tests with `ctest --preset <preset>`.

These checks match the CI pipeline.

## Continuous integration

CI builds in Valve's SteamRT SDK using VoltMod's setup action. Conan packages
and compiler output are cached from `conan.lock`. A separate job runs Ruff and
clang-format checks.

CircleCI mirrors CI and deployment as a fallback. Its `ghcr` context publishes
the runtime image, and `cs2-deploy` carries base64-encoded deployment secrets.
A `prod` push deploys twice if both providers are enabled, so disable one deploy
workflow.

Contexts are created and filled by `./.circleci/bootstrap.sh`; see
[.circleci/README.md](.circleci/README.md) for the setup walkthrough.
