#include "../Admin/Effects/Descriptors.hpp"
#include "../Core/App.hpp"
#include "../Core/Permissions.hpp"

#include <CS2Kit/Api.hpp>

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;

void RegisterAdminSelfCommands(CS2Kit::CommandManager& commands, App& app)
{
    commands.Register({
        .Name = "hide",
        .Description = "Toggle stealth-spectator mode on yourself.",
        .Usage = "!hide",
        .Permission = Flag(Permission::Hide),
        .Handler =
            [&app](CommandContext& c) {
                int slot = c.CallerSlot();
                CS2Kit::ToggleEffect(app.Effects, slot, slot, AdminSystem::Admin::Effects::Hide);
                return CommandResult{true, ""};
            },
    });
}

}  // namespace AdminSystem::Commands
