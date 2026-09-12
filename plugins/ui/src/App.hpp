#pragma once

#include "Config.hpp"
#include "Hud.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Subscription.hpp>

namespace Ui
{

/**
 * Everything this plugin owns for one Load/Unload cycle. The plugin creates it in OnLoad and
 * drops it in OnUnload, so no state survives a `meta reload`.
 *
 * Members are declared in dependency order and destroyed in reverse; each is handed the
 * collaborators it needs.
 */
struct App
{
    explicit App(VoltMod::Runtime& runtime) : Runtime(runtime) {}
    ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /** Load settings, require the addon and publish the HUD. False aborts the plugin load. */
    bool Start();

    VoltMod::Runtime& Runtime;
    ConfigManager Config;
    Ui::Hud Hud{Runtime, Config};

private:
    /** The workshop addon carrying the compiled layout; dropping it drops the requirement. */
    VoltMod::Subscription _addon;
};

}  // namespace Ui
