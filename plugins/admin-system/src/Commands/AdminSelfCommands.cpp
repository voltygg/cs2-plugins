#include "../Admin/Effects/Descriptors.hpp"
#include "../Core/Managers.hpp"
#include "../Core/Permissions.hpp"

#include <CS2Kit/Api.hpp>

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using CS2Kit::Registry;

namespace
{

const bool _registered = [] {
    Registry<CommandSpec>::Add({
        .Name = "hide",
        .Description = "Toggle stealth-spectator mode on yourself.",
        .Usage = "!hide",
        .Permission = Flag(Permission::Hide),
        .Handler =
            [](CommandContext& c) {
                int slot = c.CallerSlot();
                CS2Kit::ToggleEffect(App().Effects, slot, slot, AdminSystem::Admin::Effects::Hide);
                return CommandResult{true, ""};
            },
    });

    return true;
}();

}  // namespace

}  // namespace AdminSystem::Commands
