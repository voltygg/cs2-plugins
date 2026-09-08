---
paths:
  - "plugins/**/*.cpp"
  - "plugins/**/*.hpp"
---

# C++ conventions

## Style

- C++23, `.hpp` headers, `#pragma once`.
- Prefer `std::format`, designated initializers, and `int64_t` SteamIDs.
- `PascalCase` types and methods, `_camelCase` members, `camelCase` locals and parameters.
- Keep files around 300-350 lines when a split improves readability.

## Names and includes

- Framework names are `VoltMod::Thing`. Qualify them, or name what a .cpp uses with `using VoltMod::Player;`. Never `using namespace`; never a using-declaration in a header.
- Include the defining header. No forward declarations in headers, except a mutually owning pair declared in the plugin's single `*Types.hpp` with a comment saying why. `modgraph` exempts that filename and fails the rest.
- No anonymous namespaces. A file-local helper is a `static` function or constant at the top of the .cpp, or a private static member when it needs class state.

## Threading and blocking

- Game code runs on the main thread. No plugin threads or mutexes; VoltMod's DB and HTTP workers replay completions on the game thread.
- Database calls are async during gameplay. Blocking is only for load-time work: migrations and explicit admin reloads.
- Database TUs include `<VoltMod/Database/Api.hpp>`; the main `<VoltMod/Api.hpp>` deliberately leaves libpqxx out.

## Dependencies

- Constructor injection. No singletons, ambient lookups, or generic Services/Env bags.
- Stable services bind in constructors; per-request data (slots, descriptors) goes to methods.
- `ActionContext` carries only the resolved pair and its controllers, never `Runtime&`. A body that needs a service uses the `App&` it captured, or a small per-descriptor factory when built as static data (see `Admin/Effects/*.cpp`).

## Tests

- doctest in `plugins/<name>/tests/` via `voltmod_add_tests()`.
- Each case becomes a CTest entry, so names must not contain `[`, `]`, or `;`.
