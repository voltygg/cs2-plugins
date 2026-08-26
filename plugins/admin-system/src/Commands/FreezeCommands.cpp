#include "../Admin/AdminManager.hpp"
#include "../Admin/FreezeManager.hpp"
#include "../Core/App.hpp"
#include "../Core/ChatService.hpp"
#include "../Core/Permissions.hpp"
#include "Commands.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/StringUtils.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>

namespace AdminSystem::Commands
{

using namespace VoltMod::Commands;
using namespace VoltMod::Core;

void RegisterFreezeCommands(VoltMod::CommandManager& commands, App& app)
{
    commands.Register({
        .Name = "freeze_admin",
        .Description = "Freeze all admin privileges of another admin pending review.",
        .Permission = Flag(Permission::FreezeAdmins),
        // TargetOrSteamId: a numeric token addresses an offline admin (they are in the loaded
        // admins map); anything else resolves an online player.
        .Args = {TargetOrSteamId(), ReasonTail("reason.frozenByAdmin")},
        .Handler =
            [&app](CommandContext& c) {
                int64_t targetSteamId = c.SteamId;
                std::string targetName = c.HasTarget() ? c.Target().GetName() : std::to_string(targetSteamId);

                const auto* row = app.Admins.GetAdmin(targetSteamId);
                if (!row)
                    return c.Fail("cmd.freezeNotAdmin", {{"name", targetName}});
                targetName = row->Name;

                // CanTarget would allow self-targeting, so freezing yourself needs an explicit rejection.
                if (targetSteamId == c.Caller->GetSteamID())
                    return c.Fail("cmd.freezeSelf");

                if (!app.Access.CanTarget(c.Caller->GetSteamID(), targetSteamId))
                    return c.Fail("cmd.freezeNoOutrank", {{"name", targetName}});

                if (app.Freeze.IsFrozen(targetSteamId))
                    return c.Fail("cmd.freezeAlready", {{"name", targetName}});

                bool ok =
                    app.Freeze.Freeze(targetSteamId, targetName, c.Caller->GetSteamID(), c.Caller->GetName(), c.Reason);
                return ok ? c.Ok("cmd.freezeSuccess", {{"name", targetName}})
                          : c.Fail("cmd.freezeFailed", {{"name", targetName}});
            },
    });

    commands.Register({
        .Name = "unfreeze_admin",
        .Description = "Restore a frozen admin's privileges after reviewing their case.",
        .Permission = Flag(Permission::FreezeAdmins),
        // The target may be offline and not even resolvable as a player - it is matched against
        // the frozen list itself, so this stays a bespoke Word argument.
        .Args = {Word()},
        .Handler =
            [&app](CommandContext& c) {
                int64_t targetSteamId = 0;
                if (StringUtils::IsNumeric(c.Word))
                {
                    targetSteamId = std::stoll(c.Word);
                }
                else
                {
                    int matches = 0;
                    for (const auto& [steamId, frozen] : app.Freeze.Frozen())
                    {
                        if (StringUtils::ContainsIgnoreCase(frozen.Name, c.Word))
                        {
                            targetSteamId = steamId;
                            ++matches;
                        }
                    }
                    if (matches > 1)
                        return c.Fail("target.ambiguous", {{"token", c.Word}, {"count", std::to_string(matches)}});
                }

                auto row = app.Freeze.GetFrozen(targetSteamId);
                if (!row)
                    return c.Fail("cmd.unfreezeNone", {{"token", c.Word}});

                // Unfreeze erases the row; the name is ours because GetFrozen handed back a copy.
                bool ok = app.Freeze.Unfreeze(targetSteamId, c.Caller->GetSteamID(), c.Caller->GetName());
                return ok ? c.Ok("cmd.unfreezeSuccess", {{"name", row->Name}})
                          : c.Fail("cmd.freezeFailed", {{"name", row->Name}});
            },
    });

    commands.Register({
        .Name = "frozen_admins",
        .Description = "List admins whose privileges are currently frozen.",
        .Permission = Flag(Permission::FreezeAdmins),
        .Handler =
            [&app](CommandContext& c) {
                auto& tr = app.Runtime.Translations;
                int slot = c.CallerSlot();

                const auto& frozen = app.Freeze.Frozen();
                if (frozen.empty())
                    return c.Ok("cmd.frozenNone");

                auto& chat = app.Chat;
                chat.Reply(slot, tr.Get("cmd.frozenHeader", slot, {{"count", std::to_string(frozen.size())}}));
                for (const auto& [steamId, row] : frozen)
                {
                    std::string by = "AUTO";
                    if (row.FrozenBy != 0)
                    {
                        const auto* freezer = app.Admins.GetAdmin(row.FrozenBy);
                        by = freezer ? freezer->Name : std::to_string(row.FrozenBy);
                    }
                    chat.Reply(slot, std::format("  {} ({}) - {}: {}", row.Name, row.SteamId, by, row.Reason));
                }
                return CommandResult::Silent();
            },
    });
}

}  // namespace AdminSystem::Commands
