#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <string>
#include <utility>

namespace MainMenu
{

void RegisterCommands(App& app);

bool App::Load()
{
    if (const VoltMod::PanoramaMenuSettings& menu = Config.Get().menu; menu.panorama)
    {
        auto translated = [this](std::string key) {
            return [this, key = std::move(key)](int slot) { return Runtime.Translations.Get(key, slot); };
        };
        Layout.AddText(MainMenuLayout::HomeTitleVar, translated("home.title"));
        Layout.AddText(MainMenuLayout::HomeBodyVar, translated("home.body"));
        Layout.AddText(MainMenuLayout::HomeReportVar, translated("home.report"));

        Panorama = Runtime.UsePanorama(Layout, menu.addonId);

        ReportButton = Runtime.Screens.Pressed += [this](const VoltMod::ButtonPress& press) {
            if (press.ButtonId == MainMenuLayout::Report && Runtime.Menus.IsOpen(press.Slot))
            {
                Hub.OpenSection("report", press.Slot, Runtime.Menus);
            }
        };
    }

    RegisterCommands(*this);
    return true;
}

}  // namespace MainMenu
