#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Hooks/Api.hpp>

namespace HudLab
{

void RegisterCommands(App& app);

bool App::Start()
{
    if (!VoltMod::LoadStandardConfig(Runtime, Config, {.Addon = "hud-lab"}))
        return false;

    // Subscribing is what installs the hook, so this line is also what makes the layout's Buttons
    // report anything at all.
    _subs.push_back(Runtime.Hooks.HudClicks.Clicked += [](const VoltMod::HudClick& click) {
        VoltMod::Log::Info("hud-lab: slot {} clicked '{}'.", click.Slot, click.ButtonId);
    });

    RegisterCommands(*this);
    return true;
}

}  // namespace HudLab
