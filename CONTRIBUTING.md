# Contributing to Admin System

A monorepo of C++23 Metamod:Source plugins for CS2 community servers. Reusable
engine abstractions live in the `vendor/cs2-kit/` submodule; each plugin lives
under `plugins/<name>/`.

## Setup

One command does the setup: submodules, Conan dependencies, SDK protobuf
generation into `build/`, and a first CMake build.

```bash
./scripts/bootstrap.sh
```

On Windows, run it from Git Bash launched inside an x64 Native Tools Command
Prompt for VS so MSVC is on `PATH`.

Required tools:

- CMake 4.3.4 or newer
- Conan 2.29.1 or newer
- Ninja
- MSVC on Windows, GCC 14 on Linux (Steam Runtime sniper toolchain)

The CMake, Conan, and Ninja pins are also listed in `pyproject.toml`; `uv sync`
installs them into the project environment.

## Building

```bash
scripts/build.sh                       # default preset for your platform
scripts/build.sh windows-msvc-debug    # explicit preset
scripts/build.sh linux-steamrt-release
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

The build auto-discovers `.cpp` files under `plugins/<name>/src/` and
`vendor/cs2-kit/src/`. Add a new source file in the right tree and rebuild.

To add a new plugin, create `plugins/<new>/src/`, configs, and
`plugins/<new>/CMakeLists.txt` that calls `cs2_add_plugin(<new> ...)`. Add the
plugin with `add_subdirectory(plugins/<new>)` in the root `CMakeLists.txt`.

Add new third-party C++ dependencies to `conanfile.py`, then expose their CMake
targets in `cmake/ThirdParty.cmake`.

## Hard Constraints

- C++23.
- Main-thread only. Metamod hooks run on the game thread. Do not add threads or
  mutexes in game code; the only mutex is inside `Database`.
- `.hpp` headers, never `.h`.
- C#-style naming: `PascalCase` types/methods, `_camelCase` members,
  `camelCase` locals/params, `PascalCase` constants, `camelCase` JSON keys.
- Use `std::format`, designated initializers, and `int64_t` for SteamIDs.
- Services and managers, not singletons. Use `Engine()` for cs2-kit services and
  `App()` for plugin managers.
- Keep source files around 300-350 LOC when practical.
- Comments are rare. Add one only when the reason is non-obvious.

## cs2-kit Submodule

`vendor/cs2-kit/` is a Git submodule. If your change touches shared code there:

1. Commit inside `vendor/cs2-kit/` first and push that commit.
2. Then commit the updated submodule pointer in admin-system.

## Before You Push

- Run clang-format over C++ changes.
- Run `uvx ruff check .` for Python tooling.
- Build with `scripts/build.sh`.
- Run tests with `ctest --preset <preset>`.

Matching these locally keeps the CI pipeline green.

## Continuous Integration

CI builds the Linux plugin inside a prebuilt toolchain image
(`ghcr.io/<owner>/<repo>/build`) instead of installing a compiler and build
tools on every run. That image is the `build` stage of `deploy/Dockerfile`,
published by the **Build toolchain image** workflow
(`.github/workflows/build-toolchain.yml`) whenever the Dockerfile or that
workflow changes. Run it once manually via "Run workflow" before the first CI
run so the image exists to pull. Alongside the build, CI runs `uvx ruff check`
and a `clang-format` dry-run.
