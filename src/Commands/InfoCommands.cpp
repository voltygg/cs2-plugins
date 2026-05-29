#include "InfoCommands.hpp"
#include "../Core/Managers.hpp"
#include <CS2Kit/Core/Services.hpp>

#include "../Admin/AdminManager.hpp"
#include "../Core/ChatService.hpp"

#include <CS2Kit/Players/PlayerManager.hpp>

#include <format>

using CS2Kit::Core::Kit;

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using namespace CS2Kit::Players;
using AdminSystem::Admin::AdminManager;
using AdminSystem::Core::ChatService;

namespace
{

CommandResult HandleWho(Player* admin, const std::vector<std::string>& /*args*/)
{
    auto& plrMgr = Kit().Players;
    auto& adminMgr = Sys().Admins;
    auto players = plrMgr.GetAllPlayers();

    if (players.empty())
    {
        return {true, "No players online."};
    }

    int slot = admin->GetSlot();
    auto& chat = Sys().Chat;
    chat.Reply(slot, std::format("Online players ({}):", players.size()));
    for (auto* p : players)
    {
        if (!p)
        {
            continue;
        }
        auto style = adminMgr.GetChatStyle(p->GetSteamID());
        std::string tag = style.HasPrefix() ? style.Prefix : "-";
        chat.Reply(slot, std::format("  #{} {} [{}] (immunity {})", p->GetSlot(), p->GetName(), tag,
                                     adminMgr.GetImmunity(p->GetSteamID())));
    }
    return {true, ""};
}

CommandResult HandleAdminReload(Player* /*admin*/, const std::vector<std::string>& /*args*/)
{
    bool ok = Sys().Admins.Reload();
    return {ok, ok ? "Admins and groups reloaded from database." : "Reload failed (check logs)."};
}

}  // namespace

void RegisterInfoCommands(CommandManager& mgr)
{
    mgr.Register(CommandBuilder("who")
                     .WithAliases({"players"})
                     .WithDescription("List online players, their group prefix, and immunity.")
                     .WithUsage("!who")
                     .RequirePermission("b")
                     .WithArgs(0, 0)
                     .OnExecute(HandleWho)
                     .Build());

    mgr.Register(CommandBuilder("admin_reload")
                     .WithAliases({"reload_admins"})
                     .WithDescription("Reload admins and groups from the database without restarting.")
                     .WithUsage("!admin_reload")
                     .RequirePermission("z")
                     .WithArgs(0, 0)
                     .OnExecute(HandleAdminReload)
                     .Build());
}

}  // namespace AdminSystem::Commands
