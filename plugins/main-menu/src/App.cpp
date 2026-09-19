#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/App/PluginEntry.hpp>
#include <format>
#include <string>
#include <utility>

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
        auto translated = [this](std::string key) {
            return [this, key = std::move(key)](int slot) { return Runtime.Translations.GetOr(key, slot, key); };
        };
        Layout.AddText("home_title", translated("home.title"));
        Layout.AddText("home_body", translated("home.body"));
        Layout.AddText("home_report", translated("home.report"));

        Panorama.emplace(Runtime.PanoramaMenuServices(), Layout, menu.addonId);
        PreferPanorama = Runtime.Menus.Prefer(*Panorama);

        // The menu layout ignores ids it does not own, so the landing page's button is handled here.
        ReportButton = Runtime.Screens.Pressed += [this, button = std::format("{}_report", MainMenuLayout::Layout)](
                                                       const VoltMod::ButtonPress& press) {
            if (press.ButtonId == button && Panorama->IsOpen(press.Slot))
                Hub.OpenSection("report", press.Slot, *Panorama);
        };
    }

    RegisterCommands(*this);
    return true;
}

}  // namespace MainMenu
