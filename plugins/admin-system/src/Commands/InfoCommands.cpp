#include "../Admin/AdminManager.hpp"
#include "../Admin/FreezeManager.hpp"
#include "../Core/App.hpp"
#include "../Core/ChatService.hpp"
#include "../Core/Permissions.hpp"
#include "Commands.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>

using VoltMod::CommandContext;
using VoltMod::CommandResult;
using VoltMod::PlayerManager;
using VoltMod::Runtime;

namespace AdminSystem::Commands
{

void RegisterInfoCommands(VoltMod::CommandManager& commands, App& app)
{
    commands.Register({
        .Name = "who",
        .Aliases = {"players"},
        .Description = "List online players, their group prefix, and immunity.",
        .Permission = Flag(Permission::Hide),
        .Handler =
            [&app](CommandContext& c) {
                auto players = app.Runtime.Players.All();
                if (players.empty())
                    return c.Ok("cmd.noPlayersOnline");

                auto& adminMgr = app.Admins;
                auto& chat = app.Chat;
                int slot = c.CallerSlot();
                chat.Reply(slot, std::format("Online players ({}):", players.size()));
                for (auto* p : players)
                {
                    if (!p)
                        continue;
                    auto style = adminMgr.GetChatStyle(p->SteamId());
                    std::string tag = style.HasPrefix() ? style.Prefix : "-";
                    chat.Reply(slot, std::format("  #{} {} [{}] (immunity {})", p->Slot(), p->Name(), tag,
                                                 adminMgr.GetImmunity(p->SteamId())));
                }
                return CommandResult::Silent();
            },
    });

    commands.Register({
        .Name = "admin_reload",
        .Aliases = {"reload_admins"},
        .Description = "Reload admins and groups from the database without restarting.",
        .Permission = Flag(Permission::Root),
        .Handler =
            [&app](CommandContext& c) {
                bool ok = app.Admins.Reload();
                app.Freeze.RefreshFromDatabase();
                return ok ? c.Ok("cmd.adminReloadDone") : c.Fail("cmd.adminReloadFailed");
            },
    });
}

}  // namespace AdminSystem::Commands
