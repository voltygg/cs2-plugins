#include "../Admin/AdminManager.hpp"
#include "../Admin/FreezeManager.hpp"
#include "../Core/ChatService.hpp"
#include "../Core/Managers.hpp"
#include "../Core/Permissions.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <format>

using CS2Kit::Core::Engine;

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using CS2Kit::Registry;

namespace
{

const bool _registered = [] {
    Registry<CommandSpec>::Add({
        .Name = "who",
        .Aliases = {"players"},
        .Description = "List online players, their group prefix, and immunity.",
        .Usage = "!who",
        .Permission = Flag(Permission::Hide),
        .Handler =
            [](CommandContext& c) {
                auto players = Engine().Players.GetAllPlayers();
                if (players.empty())
                    return CommandResult{true, "No players online."};

                auto& adminMgr = App().Admins;
                auto& chat = App().Chat;
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

    Registry<CommandSpec>::Add({
        .Name = "admin_reload",
        .Aliases = {"reload_admins"},
        .Description = "Reload admins and groups from the database without restarting.",
        .Usage = "!admin_reload",
        .Permission = Flag(Permission::Root),
        .Handler =
            [](CommandContext&) {
                bool ok = App().Admins.Reload();
                App().Freeze.RefreshFromDatabase();
                return CommandResult{ok, ok ? "Admins and groups reloaded from database."
                                            : "Reload failed (check logs)."};
            },
    });

    return true;
}();

}  // namespace

}  // namespace AdminSystem::Commands
