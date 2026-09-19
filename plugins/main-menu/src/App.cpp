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

    if (const VoltMod::PanoramaMenuSettings& menu = Config.Get().menu; menu.panorama)
    {
        Panorama.emplace(Runtime.PanoramaMenuServices(), Layout, menu.addonId);
        PreferPanorama = Runtime.Menus.Prefer(*Panorama);
    }

    RegisterCommands(*this);
    return true;
}

}  // namespace MainMenu
