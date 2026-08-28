#pragma once

#include "Config.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Ui/Api.hpp>
#include <string_view>
#include <vector>

namespace UiLab
{

/**
 * Everything this plugin owns for one Load/Unload cycle. The plugin creates it in OnLoad and
 * drops it in OnUnload, so no state survives a `meta reload`.
 *
 * This is the worked example for `runtime.Ui` and `runtime.Addons`: every command is a thin
 * wrapper over one framework call, and the Panorama sources under `panorama/` are the layout
 * those calls drive.
 */
struct App
{
    explicit App(VoltMod::Runtime& runtime) : Runtime(runtime) {}

    /** Load config and register the lab commands. */
    bool Start();

    /** Spawn @p layout, replacing whatever is up, and wire its buttons. */
    VoltMod::Status Spawn(std::string_view layout);

    VoltMod::Runtime& Runtime;
    ConfigManager Config;

    /** The layout this lab is driving. It owns its entity: dropping it removes the panel, which
     *  is what stops one surviving a `meta reload`. Falsy until something spawns it. */
    VoltMod::UiPanel Layout;

    /** One lease per workshop addon required, from `uilab_addon`. */
    std::vector<VoltMod::Subscription> Addons;

private:
    /** Report a press in chat and on the panel, so a click is visibly round-tripping. */
    void OnButton(int slot, std::string_view button);

    /** Handlers for the current layout's buttons, replaced whenever it is respawned. Declared
     *  last: reverse member destruction stops the handlers before the state they capture, Layout
     *  included, goes away. */
    std::vector<VoltMod::Subscription> _buttons;
};

}  // namespace UiLab
