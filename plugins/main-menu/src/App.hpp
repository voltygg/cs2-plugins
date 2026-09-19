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

static_assert(static_cast<int>(MainMenuLayout::Tabs.size()) == MaxTabs,
              "settings cap the tabs at what the layout draws");

/** One load cycle's state; members are destroyed in reverse order. */
struct App final : VoltMod::Plugin
{
    explicit App(VoltMod::Runtime& runtime) : Plugin(runtime) {}

    bool Load() override;

    ConfigManager Config{&CleanSettings};
    HubMenu Hub{Config, Runtime.Translations, Runtime.Messages, Runtime.ConVars, Runtime.Exchange};

    VoltMod::PanoramaMenuLayout Layout{Runtime.Screens, MainMenuLayout::Layout, MainMenuLayout::Tabs.size(),
                                       MainMenuLayout::Rows.size(), MainMenuLayout::IconNames};
    std::optional<VoltMod::PanoramaMenu> Panorama;
    /** Starts sessions on Panorama while held. Declared after it, so it lets go first. */
    VoltMod::Subscription PreferPanorama;
};

}  // namespace MainMenu
