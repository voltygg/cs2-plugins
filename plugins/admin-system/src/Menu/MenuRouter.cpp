#include "Menu/MenuRouter.hpp"

#include <utility>

using VoltMod::Menu;
using VoltMod::MenuOptions;
using VoltMod::MenuSurface;

namespace AdminSystem::Menus
{

MenuRouter::MenuRouter(PanoramaMenu& panorama, VoltMod::CenterHtmlMenu& centerText)
    : _panorama(panorama), _centerText(centerText)
{}

void MenuRouter::Open(int slot, std::shared_ptr<Menu> menu, MenuOptions options)
{
    // One session per player: a menu left on the other surface would stay open and frozen.
    _panorama.CloseAll(slot);
    _centerText.CloseAll(slot);

    if (!_panorama.Open(slot, menu, options))
        _centerText.Open(slot, std::move(menu), options);
}

void MenuRouter::Open(int slot, std::shared_ptr<Menu> menu)
{
    if (!_panorama.IsOpen(slot) && !_centerText.IsOpen(slot))
    {
        Open(slot, std::move(menu), {});
        return;
    }

    SessionOf(slot).Open(slot, std::move(menu));
}

void MenuRouter::Close(int slot)
{
    SessionOf(slot).Close(slot);
}

void MenuRouter::CloseAll(int slot)
{
    SessionOf(slot).CloseAll(slot);
}

void MenuRouter::CloseAll(int slot, std::string_view replyKey)
{
    SessionOf(slot).CloseAll(slot, replyKey);
}

void MenuRouter::Prompt(int slot, std::string prompt, std::function<bool(int, std::string_view)> callback)
{
    SessionOf(slot).Prompt(slot, std::move(prompt), std::move(callback));
}

std::string MenuRouter::Translate(int slot, std::string_view key, std::string_view fallback) const
{
    return SessionOf(slot).Translate(slot, key, fallback);
}

MenuSurface& MenuRouter::SessionOf(int slot) const
{
    return _panorama.IsOpen(slot) ? static_cast<MenuSurface&>(_panorama) : _centerText;
}

}  // namespace AdminSystem::Menus
