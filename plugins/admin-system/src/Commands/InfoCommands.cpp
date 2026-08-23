#include "../Admin/AdminManager.hpp"
#include "../Admin/FreezeManager.hpp"
#include "../Core/App.hpp"
#include "../Core/ChatService.hpp"
#include "../Core/Permissions.hpp"
#include "Commands.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Runtime.hpp>
#include <format>

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;

void RegisterInfoCommands(CS2Kit::CommandManager& commands, App& app)
{
    commands.Register({
        .Name = "who",
        .Aliases = {"players"},
        .Description = "List online players, their group prefix, and immunity.",
        .Usage = "!who",
        .Permission = Flag(Permission::Hide),
        .Handler =
            [&app](CommandContext& c) {
                auto players = app.Runtime.Players.GetAllPlayers();
                if (players.empty())
                    return CommandResult{true, "No players online."};

                auto& adminMgr = app.Admins;
                auto& chat = app.Chat;
                int slot = c.CallerSlot();
                chat.Reply(slot, std::format("Online players ({}):", players.size()));
                for (auto* p : players)
                {
                    if (!p)
                        continue;
                    auto style = adminMgr.GetChatStyle(p->GetSteamID());
                    std::string tag = style.HasPrefix() ? style.Prefix : "-";
                    chat.Reply(slot, std::format("  #{} {} [{}] (immunity {})", p->GetSlot(), p->GetName(), tag,
                                                 adminMgr.GetImmunity(p->GetSteamID())));
                }
                return CommandResult{true, ""};
            },
    });

    commands.Register({
        .Name = "admin_reload",
        .Aliases = {"reload_admins"},
        .Description = "Reload admins and groups from the database without restarting.",
        .Usage = "!admin_reload",
        .Permission = Flag(Permission::Root),
        .Handler =
            [&app](CommandContext&) {
                bool ok = app.Admins.Reload();
                app.Freeze.RefreshFromDatabase();
                return CommandResult{ok,
                                     ok ? "Admins and groups reloaded from database." : "Reload failed (check logs)."};
            },
    });
}

}  // namespace AdminSystem::Commands
