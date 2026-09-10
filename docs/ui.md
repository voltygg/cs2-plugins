# Server UI

The `ui` plugin owns the Panorama HUD. Other plugins draw on it through
`Contracts::IUiHud` instead of spawning panels of their own, because one plugin has to
own the workshop addon list.

## Where things live

| Path | What |
| --- | --- |
| `plugins/<name>/panorama/screens/*.xml.j2` | One screen layout, Jinja over the framework's block library |
| `plugins/<name>/panorama/screens/*.css` | That screen's own stylesheet, also Jinja |
| `plugins/<name>/panorama/images/custom_game/<set>/*.png` | Icon sets the screen's blocks reference |
| `build/panorama/<owner>/` | Rendered layout, stylesheet and icons - not committed |
| `build/panorama/ui/include/Ui/Cs2Hud.hpp` | The HUD's derived C++ binding - not committed |

`voltmod build` renders every owner's screens into `build/panorama/` before it
configures CMake, so `Ui/Cs2Hud.hpp` exists on the `ui` plugin's include path by the time
`Hud.cpp` compiles. Nothing rendered is committed; a checkout needs the Workshop Tools
only to `compile` or `publish`, never to build the plugin.

## Render, compile, publish

```bash
uv run poe panorama              # render, compile with the Workshop Tools, install into your client
uv run poe lint                  # includes `voltmod panorama check`
```

`voltmod panorama render [OWNER]` writes `build/panorama/` alone, useful for inspecting
the derived header without the Workshop Tools. `compile` also needs them and a CS2
client (found through Steam, or `CS2_CLIENT_PATH`).

## Draw on the HUD

```cpp
#include <Contracts/IUiHud.hpp>

if (auto* hud = runtime.Exchange.Get<Contracts::IUiHud>())
    hud->SetCard(Contracts::HudCard::Third, slot,
                 {.Title = "UMP-45 | Crimson", .Subtitle = "Winston", .Value = "358 P",
                  .Icon = "ump45", .Accent = Contracts::HudAccent::Rare});
```

`Contracts::HudCard` names the cards; `Contracts::ServerCard` is the one the ui plugin draws
itself, and a plugin takes one of the others. Two plugins on the same card overwrite each
other, so pick one and say so. `SetCard` returns false when nothing could be drawn. `slot`
`Contracts::Everyone` (-1) means everyone. Strings are copied before the call returns.
`ui_hud_demo <slot>` on the server console fills the other two cards with sample content.

`Contracts::HudAccent`'s order is checked at compile time in `Hud.cpp` against the
screen's own `Cs2Ui::Hud::AccentNames`, so reordering one without the other fails the
build rather than miscolouring a card.

## Publish the addon

A client renders a layout only once it has the compiled resource, which is what the
workshop addon carries.

1. `uv run --project vendor/voltmod voltmod panorama publish build/workshop/cs2ui`
2. Upload that content directory with the CS2 Workshop Tools.
3. Put the published id in `plugins/ui/configs/settings.jsonc` as `ui.addonId`.

`panorama publish` copies every plugin's rendered tree into the same directory, so
one addon can carry this plugin's HUD and admin-system's menu screen together. Each
plugin names the published id in its own settings: `ui.addonId` here,
`menu.addonId` in admin-system.
