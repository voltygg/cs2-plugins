#include "../Admin/AdminManager.hpp"
#include "../Admin/FreezeManager.hpp"
#include "../Core/App.hpp"
#include "../Core/Permissions.hpp"
#include "Commands.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>
#include <string>

using VoltMod::Caller;
using VoltMod::Reply;
using VoltMod::Result;

namespace AdminSystem::Commands
{

void RegisterInfoCommands(VoltMod::CommandManager& commands, App& app)
{
    commands.Add("who")
        .Alias("players")
        .Describe("List online players, their group prefix, and immunity.")
        .Permission(Flag(Permission::Hide))
        .Run([&app](Caller c) -> Result<Reply> {
            auto players = app.Runtime.Players.All();
            if (players.empty())
                return c.Ok("cmd.noPlayersOnline");

            auto& adminMgr = app.Admins;
            c.Say("cmd.whoHeader", {{"count", std::to_string(players.size())}});
            for (auto* p : players)
            {
                if (!p)
                    continue;
                auto style = adminMgr.GetChatStyle(p->SteamId());
                std::string tag = style.HasPrefix() ? style.Prefix : "-";
                c.SayRaw(std::format("  #{} {} [{}] (immunity {})", p->Slot(), p->Name(), tag,
                                     adminMgr.GetImmunity(p->SteamId())));
            }
            return Reply::Silent();
        });

    commands.Add("admin_reload")
        .Alias("reload_admins")
        .Describe("Reload admins and groups from the database without restarting.")
        .Permission(Flag(Permission::Root))
        .Run([&app](Caller c) -> Result<Reply> {
            bool ok = app.Admins.Reload();
            app.Freeze.RefreshFromDatabase();
            return ok ? c.Ok("cmd.adminReloadDone") : c.Fail("cmd.adminReloadFailed");
        });
}

}  // namespace AdminSystem::Commands
