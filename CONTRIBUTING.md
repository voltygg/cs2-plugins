# Contributing to Admin System

A C++23 Metamod:Source plugin for CS2 community servers. Reusable engine
abstractions live in the **cs2-kit** submodule (`vendor/cs2-kit/`); the plugin
itself is everything under `src/`. This guide gets you building and shipping a
change quickly. For the full Windows walkthrough see
[docs/local-development.md](docs/local-development.md).

## Setup

One command does the whole setup (submodules, Python + C++ deps, protobuf
headers, and a first build):

```bash
./scripts/bootstrap.sh
```

On Windows, run it from **Git Bash launched inside an x64 Native Tools Command
Prompt for VS** so MSVC is on `PATH`. Under the hood it runs
`git submodule update --init --recursive`, `uv sync`, `vcpkg install`, the
protobuf `protoc` generation, and `scripts/build.sh`.

## Building

After the initial bootstrap, rebuild with:

```bash
scripts/build.sh        # configure + ambuild + deploy (run from the x64 Native Tools shell)
```

Or develop in Visual Studio: generate a solution with
`uv run python configure.py --gen=vs --vs-version 19` and open the `.sln`.

Build output lands in `objdir/src/`.

### Adding a source file

The build auto-discovers sources. To add code, just **drop a `.cpp` under
`src/`** (or `vendor/cs2-kit/src/` for shared code) — `src/AMBuilder` walks
those trees and picks it up.

> Adding a **new** file means re-running configure so AMBuild sees it:
> `scripts/build.sh` does this for you (it always runs `configure.py`). If you
> drive `ambuild` directly from `objdir/`, re-run `uv run python configure.py`
> first. Editing an existing file needs no reconfigure.

## Hard constraints

These are non-negotiable — a change that violates them will be rejected:

- **C++23**, compiled with MSVC `/std:c++latest`. Be exact about types,
  includes, and namespaces.
- **Main-thread only.** Every Metamod hook runs on the game thread. Do **not**
  spawn threads or add mutexes in game code. The **only** mutex in the codebase
  is inside the `Database` class. (`HttpClient` already runs libcurl off-thread
  and marshals completions back to the main thread — leave that pattern intact.)
- **`.hpp` headers** (never `.h`).
- **C#-style naming:** `PascalCase` types/methods, `_camelCase` members,
  `camelCase` locals/params, `PascalCase` constants, `camelCase` JSON keys.
- **`std::format`** for string formatting; **designated initializers** for
  struct construction; `int64_t` for SteamIDs.
- **Services / Managers, not singletons.** cs2-kit services are reached through
  `Kit()` (`CS2Kit::Core::Services`); plugin-side managers through `Sys()`
  (the `Managers` struct). Both are built in `OnLoad` and torn down on unload —
  there are no process-lifetime singletons. Don't add `static` global state.
- Keep it **simple and newcomer-readable**, and keep source files under
  ~300-350 LOC (split by responsibility when they grow past that).
- **Comments are rare.** Add one only when the *why* is non-obvious. Names carry
  the meaning.

## cs2-kit submodule

`vendor/cs2-kit/` is a Git submodule. If your change touches shared code there:

1. Commit **inside `vendor/cs2-kit/` first** and push that commit.
2. Then commit the updated submodule pointer in admin-system.

Committing the pointer before the cs2-kit commit is reachable leaves the
superproject pointing at a commit nobody else can fetch — always submodule
first. (The `/commit` skill handles this ordering for you.)

## Before you push

- Run **clang-format** over your C++ changes (config in `.clang-format`):
  `clang-format -i <files>`.
- Run **ruff** over the build tooling: `uvx ruff check .`.
- Make sure it still builds (`scripts/build.sh`).

CI runs the Docker build plus a ruff + clang-format dry-run, so matching these
locally keeps the pipeline green.
