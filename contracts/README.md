# Contracts

Interfaces the plugins in this repository offer each other through VoltMod's `ServiceExchange`.
It is a header-only CMake target, `cs2-contracts`, not a plugin: the implementation lives in
whichever plugin publishes the interface.

| Interface | Published by | Used by | What it offers |
| --- | --- | --- | --- |
| `IAdminActions` | admin-system | anticheat | Automated bans and admin alerts |
| `IPermissions` | admin-system | stronghold | Whether a SteamID holds a permission |
| `IMenuSection` | admin-system (`admin`, `report`), stronghold (`stronghold`) | main-menu | A menu entry that opens the plugin's own UI |

## Using one

Link the target and ask the exchange each time you need it, because the publisher may load later
or unload in between:

```cmake
target_link_libraries(my-plugin PRIVATE cs2-contracts)
```

```cpp
#include <Contracts/IAdminActions.hpp>

if (auto* admin = Runtime.Exchange.Get<Contracts::IAdminActions>())
    admin->AlertAdmins(steamId, "aimbot", score);
```

`Get` returns nullptr when nothing is published. Publish with `Exchange.Publish<T>(this)` in
`Load`, or `Publish<T>(this, id)` for an interface with several publishers, and unpublish on
unload.

## Changing one

Each plugin has its own heap, so nothing that owns memory crosses the boundary: parameters are
views the callee must not keep, and return values are trivially copyable. Any change to a vtable,
or to what a parameter means, bumps the `/N` in that interface's `InterfaceName`, so an old
consumer gets nullptr instead of a mismatched vtable.
