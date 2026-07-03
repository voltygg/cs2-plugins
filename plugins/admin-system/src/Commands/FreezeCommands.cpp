#include "FreezeCommands.hpp"

#include "../Admin/AdminManager.hpp"
#include "../Admin/FreezeManager.hpp"
#include "../Core/ChatService.hpp"
#include "../Core/Managers.hpp"
#include "../Core/Permissions.hpp"
#include "CommandHelpers.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using namespace CS2Kit::Players;
using namespace CS2Kit::Utils;
using namespace AdminSystem::Commands::Helpers;
using CS2Kit::Core::Engine;

namespace
{

/**
 * Resolve a freeze target to a known admin: a numeric token is a SteamID (works for offline
 * admins - they are in the loaded admins map), anything else resolves an online player.
 * Returns false with the reply already written into @p out.
 */
bool ResolveAdminTarget(Player* caller, const std::string& token, int64_t& outSteamId, std::string& outName,
                        CommandResult& out)
{
    auto& tr = Engine().Translations;
    if (StringUtils::IsNumeric(token))
    {
        outSteamId = std::stoll(token);
        outName = token;
    }
    else
    {
        std::string err;
        Player* target = ResolveSingle(token, caller, err);
        if (!target)
        {
            out = {false, err};
            return false;
        }
        outSteamId = target->GetSteamID();
        outName = target->GetName();
    }

    if (const auto* row = App().Admins.GetAdmin(outSteamId))
    {
        outName = row->Name;
        return true;
    }
    out = {false, tr.Get("cmd.freezeNotAdmin", caller->GetSlot(), {{"name", outName}})};
    return false;
}

CommandResult HandleFreezeAdmin(Player* admin, const std::vector<std::string>& args)
{
    auto& tr = Engine().Translations;
    int slot = admin->GetSlot();

    int64_t targetSteamId = 0;
    std::string targetName;
    CommandResult err;
    if (!ResolveAdminTarget(admin, args[0], targetSteamId, targetName, err))
        return err;

    // CanTarget would allow self-targeting, so freezing yourself needs an explicit rejection.
    if (targetSteamId == admin->GetSteamID())
        return {false, tr.Get("cmd.freezeSelf", slot)};

    if (!App().Admins.CanTarget(admin->GetSteamID(), targetSteamId))
        return {false, tr.Get("cmd.freezeNoOutrank", slot, {{"name", targetName}})};

    if (App().Freeze.IsFrozen(targetSteamId))
        return {false, tr.Get("cmd.freezeAlready", slot, {{"name", targetName}})};

    std::string reason = JoinReason(args, 1, tr.Get("reason.frozenByAdmin"));
    bool ok = App().Freeze.Freeze(targetSteamId, targetName, admin->GetSteamID(), admin->GetName(), reason);
    return {ok, tr.Get(ok ? "cmd.freezeSuccess" : "cmd.freezeFailed", slot, {{"name", targetName}})};
}

CommandResult HandleUnfreezeAdmin(Player* admin, const std::vector<std::string>& args)
{
    auto& tr = Engine().Translations;
    int slot = admin->GetSlot();

    int64_t targetSteamId = 0;
    if (StringUtils::IsNumeric(args[0]))
    {
        targetSteamId = std::stoll(args[0]);
    }
    else
    {
        int matches = 0;
        for (const auto& [steamId, frozen] : App().Freeze.Frozen())
        {
            if (StringUtils::ContainsIgnoreCase(frozen.Name, args[0]))
            {
                targetSteamId = steamId;
                ++matches;
            }
        }
        if (matches > 1)
            return {false, tr.Get("target.ambiguous", slot, {{"token", args[0]}, {"count", std::to_string(matches)}})};
    }

    const auto* row = App().Freeze.GetFrozen(targetSteamId);
    if (!row)
        return {false, tr.Get("cmd.unfreezeNone", slot, {{"token", args[0]}})};

    std::string targetName = row->Name;
    bool ok = App().Freeze.Unfreeze(targetSteamId, admin->GetSteamID(), admin->GetName());
    return {ok, tr.Get(ok ? "cmd.unfreezeSuccess" : "cmd.freezeFailed", slot, {{"name", targetName}})};
}

CommandResult HandleFrozenAdmins(Player* admin, const std::vector<std::string>& /*args*/)
{
    auto& tr = Engine().Translations;
    int slot = admin->GetSlot();

    const auto& frozen = App().Freeze.Frozen();
    if (frozen.empty())
        return {true, tr.Get("cmd.frozenNone", slot)};

    auto& chat = App().Chat;
    chat.Reply(slot, tr.Get("cmd.frozenHeader", slot, {{"count", std::to_string(frozen.size())}}));
    for (const auto& [steamId, row] : frozen)
    {
        std::string by = "AUTO";
        if (row.FrozenBy != 0)
        {
            const auto* freezer = App().Admins.GetAdmin(row.FrozenBy);
            by = freezer ? freezer->Name : std::to_string(row.FrozenBy);
        }
        chat.Reply(slot, std::format("  {} ({}) — {}: {}", row.Name, row.SteamId, by, row.Reason));
    }
    return {true, ""};
}

}  // namespace

void RegisterFreezeCommands(CommandManager& mgr)
{
    mgr.Register(CommandBuilder("freeze_admin")
                     .WithDescription("Freeze all admin privileges of another admin pending review.")
                     .WithUsage("!freeze_admin <target|steamId> [reason]")
                     .RequirePermission(Flag(Permission::FreezeAdmins))
                     .WithArgs(1)
                     .OnExecute(HandleFreezeAdmin)
                     .Build());

    mgr.Register(CommandBuilder("unfreeze_admin")
                     .WithDescription("Restore a frozen admin's privileges after reviewing their case.")
                     .WithUsage("!unfreeze_admin <steamId|name>")
                     .RequirePermission(Flag(Permission::FreezeAdmins))
                     .WithArgs(1)
                     .OnExecute(HandleUnfreezeAdmin)
                     .Build());

    mgr.Register(CommandBuilder("frozen_admins")
                     .WithDescription("List admins whose privileges are currently frozen.")
                     .WithUsage("!frozen_admins")
                     .RequirePermission(Flag(Permission::FreezeAdmins))
                     .WithArgs(0, 0)
                     .OnExecute(HandleFrozenAdmins)
                     .Build());
}

}  // namespace AdminSystem::Commands
