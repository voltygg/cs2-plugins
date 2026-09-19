#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/App/PluginEntry.hpp>

VOLTMOD_PLUGIN(MainMenu::App);

namespace MainMenu
{

void RegisterCommands(App& app);

bool App::Load()
{
    if (!VoltMod::LoadStandardConfig(Runtime, Config))
        return false;

    if (const MenuSettings& menu = Config.Get().menu; menu.panorama)
    {
        Panorama.emplace(VoltMod::PanoramaMenu::Services{.Scheduler = Runtime.Scheduler,
                                                         .Slots = Runtime.Slots,
                                                         .Freeze = Runtime.Freeze,
                                                         .ChatInput = Runtime.Hooks.ChatInput,
                                                         .Translations = Runtime.Translations,
                                                         .Policy = Runtime.Policy,
                                                         .Screens = Runtime.Screens,
                                                         .Addons = Runtime.Addons},
                         Layout, menu.addonId);
        PreferPanorama = Runtime.Menus.Prefer(*Panorama);
    }

    RegisterCommands(*this);
    return true;
}

}  // namespace MainMenu
