# ui

The server HUD. It owns the Panorama screen (`panorama/screens/cs2_hud.xml.j2`), the
workshop addon every client downloads, and publishes `Contracts::IUiHud` so other
plugins draw on it instead of spawning panels of their own.

Three cards top-left and a toast at the bottom. Card 0 is the plugin's own Server card,
the configured name over a live player count; cards 1 and 2 are for other plugins. admin-system draws its
menu on the same addon, so this plugin must be loaded for `!admin` to open anything.

## Configuration

`addons/ui/configs/settings.jsonc`:

| Key | Default | Purpose |
| --- | --- | --- |
| `ui.addonId` | `0` | Workshop addon carrying the compiled layout; 0 requires nothing of clients |
| `ui.serverName` | `meat.gg` | Title of the Server card |
| `ui.toastDurationMs` | `4000` | How long a toast stays up when the caller passes 0 |

## Commands

`ui_hud_demo <slot>` (server console, -1 for everyone) fills cards 1 and 2 with
sample content and fires a toast, so a layout change can be seen without the plugin that
would normally drive them.

See [Server UI](../../docs/ui.md) for generation and addon publishing.
