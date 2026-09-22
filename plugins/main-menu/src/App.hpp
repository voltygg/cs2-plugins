#pragma once

#include "Config.hpp"
#include "HubMenu.hpp"

#include <Ui/MainMenu.hpp>
#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Signals/Subscription.hpp>
#include <VoltMod/Menu/PanoramaMenu.hpp>
#include <VoltMod/Menu/PanoramaMenuLayout.hpp>
#include <optional>

namespace MainMenu
{

static_assert(static_cast<int>(MainMenuLayout::Tabs.size()) == MaxTabs + 1,
              "the layout draws the configured tabs plus Settings");

/** One load cycle's state; members are destroyed in reverse order. */
struct App final : VoltMod::Plugin
{
    explicit App(VoltMod::Runtime& runtime) : Plugin(runtime) {}

    bool Load() override;

    ConfigManager Config{&CleanSettings};
    HubMenu Hub{Config, Runtime.Translations, Runtime.Messages, Runtime.ConVars, Runtime.Exchange, Runtime.Menus};

    VoltMod::PanoramaMenuLayout Layout{Runtime.Screens, MainMenuLayout::Layout, MainMenuLayout::Tabs.size(),
                                       MainMenuLayout::Rows.size(), MainMenuLayout::IconSetNames};
    std::optional<VoltMod::PanoramaMenu> Panorama;
    /** Routes menu sessions to Panorama while held; declared after it so it releases first. */
    VoltMod::Subscription PreferPanorama;
    /** The home page's report button; the layout does not own that id. */
    VoltMod::Subscription ReportButton;
};

}  // namespace MainMenu
