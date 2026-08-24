# Contributing to Admin System

A monorepo of C++23 Metamod:Source plugins for CS2 community servers. Reusable
engine abstractions live in [voltmod](https://github.com/voltygg/voltmod),
consumed as a Conan package; each plugin lives under `plugins/<name>/`.

## Setup

Two commands: `uv sync` provisions the toolchain, `bootstrap` installs the kit's
Conan profiles and the public remote and runs a first build.

```bash
uv sync
uv run poe bootstrap
```

On Windows this auto-loads the MSVC environment via `vcvars64.bat`, so a plain
shell works; no x64 Native Tools prompt is required.

Required tools:

- CMake 4.3.4 or newer
- Conan 2.29.1 or newer
- Ninja
- MSVC on Windows, GCC 14 on Linux (Steam Runtime sniper toolchain)

The CMake, Conan, Ninja and clang-format pins live in the `voltmod` Python
distribution, which `pyproject.toml` depends on; `uv sync` installs them all.

## Building

```bash
uv run poe build                       # default preset for your platform
uv run poe build windows-msvc-debug    # explicit preset
uv run poe build-linux
```

Build output lands in:

```text
build/<preset>/plugins/<name>/<platform-arch>/
```

Available presets:

- `linux-steamrt-release`
- `linux-steamrt-debug`
- `windows-msvc-release`
- `windows-msvc-debug`

## Adding Code

The build auto-discovers `.cpp` files under `plugins/<name>/src/`. Add a new
source file there and rebuild.

To add a new plugin, create `plugins/<new>/src/`, configs, and
`plugins/<new>/CMakeLists.txt` that calls `voltmod_add_plugin(<new> VERSION <version> ...)`. Add the
plugin with `add_subdirectory(plugins/<new>)` in the root `CMakeLists.txt`.

Add new third-party C++ dependencies to `conanfile.py`, then `find_package`
them in the root `CMakeLists.txt` and link their imported targets (e.g.
`libpqxx::pqxx`) in the plugin's `CMakeLists.txt`.

## Hard Constraints

- C++23.
- Main-thread only. Metamod hooks run on the game thread. Do not add threads or
  mutexes in game code; the only mutex is inside `Database`.
- `.hpp` headers, never `.h`.
- C#-style naming: `PascalCase` types/methods, `_camelCase` members,
  `camelCase` locals/params, `PascalCase` constants, `camelCase` JSON keys.
- Use `std::format`, designated initializers, and `int64_t` for SteamIDs.
- Services and managers, not singletons. Use `Engine()` for voltmod services and
  `App()` for plugin managers.
- Keep source files around 300-350 LOC when practical.
- Comments are rare. Add one only when the reason is non-obvious.

## Changing voltmod alongside a plugin

The kit is a Conan package, not a subdirectory, so point Conan at a local
checkout while you work on both:

`vendor/voltmod` is a submodule for exactly this (`git submodule update --init` on an
older clone); any sibling checkout works too.

```bash
conan editable add vendor/voltmod
uv run poe build          # picks up kit edits directly
conan editable remove voltmod
```

It is a checkout, not a version - without the editable, the build uses the Conan package
from `conanfile.py`.

Land the kit change first (its own repo, its own PR). A `v*` tag there publishes
a new package; bump the range in `conanfile.py` only if the new version falls
outside `[~1]`.

## Before You Push

- Run clang-format over C++ changes.
- Run `uv run poe lint` for Python tooling.
- Build with `uv run poe build`.
- Run tests with `ctest --preset <preset>`.

Matching these locally keeps the CI pipeline green.

## Continuous Integration

CI builds in Valve's SteamRT SDK using VoltMod's setup action. Conan packages and
compiler output are cached from `conan.lock`. A separate job runs Ruff and
clang-format checks.

CircleCI mirrors CI and deploy as a fallback. Its `ghcr` context publishes the
runtime image; `cs2-deploy` carries the base64 deploy secrets. A `prod` push will
deploy twice if both providers are enabled, so disable one deploy workflow.

Contexts are created and filled by `./.circleci/bootstrap.sh`; see
[.circleci/README.md](.circleci/README.md) for the full setup walkthrough.
