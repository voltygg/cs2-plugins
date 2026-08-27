#include "../Admin/AdminManager.hpp"
#include "../Admin/FreezeManager.hpp"
#include "../Core/App.hpp"
#include "../Core/Permissions.hpp"
#include "Commands.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Strings.hpp>
#include <format>
#include <string>

using VoltMod::Caller;
using VoltMod::Reply;
using VoltMod::Result;
using VoltMod::Strings;

namespace Args = VoltMod::Args;

namespace AdminSystem::Commands
{

void RegisterFreezeCommands(VoltMod::CommandManager& commands, App& app)
{
    commands.Add("freeze_admin")
        .Describe("Freeze all admin privileges of another admin pending review.")
        .Permission(Flag(Permission::FreezeAdmins))
        // PlayerOrSteamId: an offline admin is addressed by the bare SteamID64 they are
        // stored under, anyone online by the usual selector grammar.
        .Run([&app](Caller c, Args::PlayerOrSteamId who, Args::Opt<Args::Rest> why) -> Result<Reply> {
            std::string targetName = who.Online ? who.Online->Name() : std::to_string(who.SteamId);

            const auto* row = app.Admins.GetAdmin(who.SteamId);
            if (!row)
                return c.Fail("cmd.freezeNotAdmin", {{"name", targetName}});
            targetName = row->Name;

            // Targeting yourself is allowed in general, so freezing yourself needs an
            // explicit rejection here.
            if (who.SteamId == c.Player->SteamId())
                return c.Fail("cmd.freezeSelf");

            if (!app.Access.CanTarget(c.Player->SteamId(), who.SteamId))
                return c.Fail("cmd.freezeNoOutrank", {{"name", targetName}});

            if (app.Freeze.IsFrozen(who.SteamId))
                return c.Fail("cmd.freezeAlready", {{"name", targetName}});

            const std::string reason = ReasonOr(c, why, "reason.frozenByAdmin");
            bool ok = app.Freeze.Freeze(who.SteamId, targetName, c.Player->SteamId(), c.Player->Name(), reason);
            return ok ? c.Ok("cmd.freezeSuccess", {{"name", targetName}})
                      : c.Fail("cmd.freezeFailed", {{"name", targetName}});
        });

    commands.Add("unfreeze_admin")
        .Describe("Restore a frozen admin's privileges after reviewing their case.")
        .Permission(Flag(Permission::FreezeAdmins))
        // The target may be offline and not even resolvable as a player - it is matched
        // against the frozen list itself, so this stays a bespoke Word argument.
        .Run([&app](Caller c, Args::Word token) -> Result<Reply> {
            int64_t targetSteamId = 0;
            if (Strings::IsNumeric(token.Value))
            {
                targetSteamId = std::stoll(token.Value);
            }
            else
            {
                int matches = 0;
                for (const auto& [steamId, frozen] : app.Freeze.Frozen())
                {
                    if (Strings::ContainsIgnoreCase(frozen.Name, token.Value))
                    {
                        targetSteamId = steamId;
                        ++matches;
                    }
                }
                if (matches > 1)
                    return c.Fail("target.ambiguous", {{"token", token.Value}, {"count", std::to_string(matches)}});
            }

            auto row = app.Freeze.GetFrozen(targetSteamId);
            if (!row)
                return c.Fail("cmd.unfreezeNone", {{"token", token.Value}});

            // Unfreeze erases the row; the name is ours because GetFrozen handed back a copy.
            bool ok = app.Freeze.Unfreeze(targetSteamId, c.Player->SteamId(), c.Player->Name());
            return ok ? c.Ok("cmd.unfreezeSuccess", {{"name", row->Name}})
                      : c.Fail("cmd.freezeFailed", {{"name", row->Name}});
        });

    commands.Add("frozen_admins")
        .Describe("List admins whose privileges are currently frozen.")
        .Permission(Flag(Permission::FreezeAdmins))
        .Run([&app](Caller c) -> Result<Reply> {
            const auto& frozen = app.Freeze.Frozen();
            if (frozen.empty())
                return c.Ok("cmd.frozenNone");

            c.Say("cmd.frozenHeader", {{"count", std::to_string(frozen.size())}});
            for (const auto& [steamId, row] : frozen)
            {
                std::string by = "AUTO";
                if (row.FrozenBy != 0)
                {
                    const auto* freezer = app.Admins.GetAdmin(row.FrozenBy);
                    by = freezer ? freezer->Name : std::to_string(row.FrozenBy);
                }
                c.SayRaw(std::format("  {} ({}) - {}: {}", row.Name, row.SteamId, by, row.Reason));
            }
            return Reply::Silent();
        });
}

}  // namespace AdminSystem::Commands
