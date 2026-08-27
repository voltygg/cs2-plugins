#pragma once

#include "Config.hpp"

#include <VoltMod/Api.hpp>
#include <vector>

namespace HudLab
{

/**
 * Everything this plugin owns for one Load/Unload cycle. The plugin creates it in OnLoad and
 * drops it in OnUnload, so no state survives a `meta reload`.
 *
 * This is the worked example for `runtime.World.CustomHud`, `runtime.Hooks.HudClicks` and
 * `runtime.Addons`: every command below is a thin wrapper over one framework call, and the
 * Panorama sources under `panorama/` are the layout those calls drive.
 */
struct App
{
    explicit App(VoltMod::Runtime& runtime) : Runtime(runtime) {}

    /** Load config, subscribe to clicks, and register the lab commands. */
    bool Start();

    VoltMod::Runtime& Runtime;
    ConfigManager Config;

    /** The layout this lab is driving. A ref, not a wrapper: `CustomHud::Get` resolves it where
     *  it is used, and a map change or a `Kill` leaves it harmlessly empty. */
    VoltMod::EntityRef Layout;

private:
    /** Listener registrations, released together. Declared last: reverse member destruction stops
     *  the handlers before the state they capture goes away. */
    std::vector<VoltMod::Subscription> _subs;
};

}  // namespace HudLab
