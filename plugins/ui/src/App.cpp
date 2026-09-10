#include "App.hpp"

#include <VoltMod/Api.hpp>

namespace Log = VoltMod::Log;

namespace Ui
{

void RegisterCommands(VoltMod::CommandManager& commands, App& app);

App::~App()
{
    // Stop answering other plugins' MetaFactory queries before the screen it draws on goes.
    ServerHud.Unpublish();
}

bool App::Start()
{
    if (!VoltMod::LoadStandardConfig(Runtime, Config, {.Addon = AddonName}))
        return false;

    const uint64_t addonId = Config.Get().ui.addonId;
    if (addonId != 0)
    {
        if (auto required = Runtime.Addons.Require(addonId))
            _addon = std::move(*required);
        else
            Log::Warn("HUD addon {} not required ({}); players without the layout see nothing.", addonId,
                      required.error().Detail);
    }

    ServerHud.Start();
    ServerHud.Publish();
    RegisterCommands(Runtime.Commands, *this);
    return true;
}

}  // namespace Ui
