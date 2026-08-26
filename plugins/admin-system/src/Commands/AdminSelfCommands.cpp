#include "../Admin/Effects/Descriptors.hpp"
#include "../Core/App.hpp"
#include "../Core/Permissions.hpp"
#include "Commands.hpp"

#include <VoltMod/Api.hpp>

using VoltMod::CommandContext;
using VoltMod::CommandResult;

namespace AdminSystem::Commands
{

void RegisterAdminSelfCommands(VoltMod::CommandManager& commands, App& app)
{
    commands.Register({
        .Name = "hide",
        .Description = "Toggle stealth-spectator mode on yourself.",
        .Permission = Flag(Permission::Hide),
        .Handler =
            [&app](CommandContext& c) {
                int slot = c.CallerSlot();
                app.PlayerEffects.Toggle(slot, slot, AdminSystem::Admin::Effects::Hide);
                return CommandResult::Silent();
            },
    });
}

}  // namespace AdminSystem::Commands
