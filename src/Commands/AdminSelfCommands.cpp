#include "AdminSelfCommands.hpp"

#include "../Admin/Effects/Hide.hpp"

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using namespace CS2Kit::Players;

namespace
{

CommandResult HandleHide(Player* admin, const std::vector<std::string>& /*args*/)
{
    AdminSystem::Admin::Effects::ToggleHide(admin->GetSlot());
    return {true, ""};
}

}  // namespace

void RegisterAdminSelfCommands(CommandManager& mgr)
{
    mgr.Register(CommandBuilder("hide")
                     .WithDescription("Toggle stealth-spectator mode on yourself.")
                     .WithUsage("!hide")
                     .RequirePermission("b")
                     .WithArgs(0, 0)
                     .OnExecute(HandleHide)
                     .Build());
}

}  // namespace AdminSystem::Commands
