#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/App/PluginEntry.hpp>
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
            return [this, key = std::move(key)](int slot) { return Runtime.Translations.Get(key, slot); };
        };
        Layout.AddText(MainMenuLayout::HomeTitleVar, translated("home.title"));
        Layout.AddText(MainMenuLayout::HomeBodyVar, translated("home.body"));
        Layout.AddText(MainMenuLayout::HomeReportVar, translated("home.report"));

        Panorama.emplace(Runtime.PanoramaMenuServices(), Layout, menu.addonId);
        PreferPanorama = Runtime.Menus.Prefer(*Panorama);

        ReportButton = Runtime.Screens.Pressed += [this](const VoltMod::ButtonPress& press) {
            if (press.ButtonId == MainMenuLayout::Report && Panorama->IsOpen(press.Slot))
                Hub.OpenSection("report", press.Slot, *Panorama);
        };
    }

    RegisterCommands(*this);
    return true;
}

}  // namespace MainMenu
