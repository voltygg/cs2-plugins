# Contributing to Admin System

A monorepo of C++23 Metamod:Source plugins for CS2 community servers. Reusable
engine abstractions live in the `vendor/cs2-kit/` submodule; each plugin lives
under `plugins/<name>/`.

## Setup

One command does the setup: submodules, Conan dependencies, SDK protobuf
generation into `build/`, and a first CMake build.

```bash
uv run poe bootstrap
```

On Windows this auto-loads the MSVC environment via `vcvars64.bat`, so a plain
shell works; no x64 Native Tools prompt is required.

Required tools:

- CMake 4.3.4 or newer
- Conan 2.29.1 or newer
- Ninja
- MSVC on Windows, GCC 14 on Linux (Steam Runtime sniper toolchain)

The CMake, Conan, and Ninja pins are also listed in `pyproject.toml`; `uv sync`
installs them into the project environment.

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

The build auto-discovers `.cpp` files under `plugins/<name>/src/` and
`vendor/cs2-kit/src/`. Add a new source file in the right tree and rebuild.

To add a new plugin, create `plugins/<new>/src/`, configs, and
`plugins/<new>/CMakeLists.txt` that calls `cs2_add_plugin(<new> ...)`. Add the
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
- Run `uv run poe lint` for Python tooling.
- Build with `uv run poe build`.
- Run tests with `ctest --preset <preset>`.

Matching these locally keeps the CI pipeline green.

## Continuous Integration

CI builds the Linux plugin inside a prebuilt toolchain image
(`ghcr.io/<owner>/<repo>/cs2-plugin-toolchain:latest`) instead of installing a
compiler and build tools on every run. That image is the `build` stage of
`deploy/Dockerfile`, published by the **Build toolchain image** workflow
(`.github/workflows/build-toolchain.yml`) whenever the Dockerfile or that
workflow changes. Run it once manually via "Run workflow" before the first CI
run so the image exists to pull. Alongside the build, CI runs `uv run poe lint`
and a `clang-format` dry-run.

CircleCI (`.circleci/config.yml`) mirrors all three workflows (CI, toolchain
image, deploy) so either provider can run when the other is out of minutes.
CircleCI has no ambient `GITHUB_TOKEN`, so its jobs authenticate to GHCR with a
PAT stored in the org-level `ghcr` context (`GHCR_USERNAME`, `GHCR_TOKEN`);
deploy secrets live in the `cs2-deploy` context as base64 (`SSH_KEY_B64`,
`SERVER_ENV_<ID>_B64`) because CircleCI env vars do not preserve newlines. The
toolchain image must exist on GHCR before either provider's CI can run -
bootstrap it from CircleCI by triggering a pipeline with the `force-toolchain`
parameter set to true. Note that a `prod` push while both providers' deploys
are enabled deploys twice; disable one side (GHA: disable the workflow in the
Actions UI; CircleCI: comment out the `deploy` workflow trigger).
