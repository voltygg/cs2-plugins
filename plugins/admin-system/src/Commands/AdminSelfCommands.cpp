#include "AdminSelfCommands.hpp"

#include "../Admin/Effects/Descriptors.hpp"
#include "../Admin/Effects/EffectAction.hpp"
#include "../Core/Permissions.hpp"

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using namespace CS2Kit::Players;

namespace
{

CommandResult HandleHide(Player* admin, const std::vector<std::string>& /*args*/)
{
    int slot = admin->GetSlot();
    AdminSystem::Admin::Effects::Run(slot, slot, AdminSystem::Admin::Effects::Hide);
    return {true, ""};
}

}  // namespace

void RegisterAdminSelfCommands(CommandManager& mgr)
{
    mgr.Register(CommandBuilder("hide")
                     .WithDescription("Toggle stealth-spectator mode on yourself.")
                     .WithUsage("!hide")
                     .RequirePermission(Flag(Permission::Hide))
                     .WithArgs(0, 0)
                     .OnExecute(HandleHide)
                     .Build());
}

}  // namespace AdminSystem::Commands
