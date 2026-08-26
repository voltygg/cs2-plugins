#include "AdminActionsService.hpp"

#include "../Admin/Access.hpp"
#include "../Database/Entities/Ban.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "Permissions.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/SteamId.hpp>
#include <string>

namespace Log = VoltMod::Log;
using Contracts::BanResult;

namespace AdminSystem::Core
{

void AdminActionsService::Publish()
{
    _rt.Exchange.Publish<Contracts::IAdminActions>(this);
}

void AdminActionsService::Unpublish()
{
    _rt.Exchange.Unpublish<Contracts::IAdminActions>();
}

BanResult AdminActionsService::Ban(int64_t steamId, int64_t durationSec, std::string_view reason)
{
    if (!VoltMod::SteamId::IsValid(steamId))
    {
        Log::Warn("IAdminActions::Ban: {} is not a SteamID64.", steamId);
        return BanResult::InvalidSteamId;
    }

    Database::Ban ban;
    ban.TargetSteamId = steamId;
    ban.AdminSteamId = 0;
    ban.AdminName = "AntiCheat";
    ban.Reason = std::string(reason);
    ban.Duration = durationSec;
    if (auto* target = _rt.Players.BySteamId(steamId))
    {
        ban.TargetName = target->Name();
        ban.TargetIp = target->Ip();
    }

    if (!_punishments.IssueBan(ban))
    {
        Log::Warn("IAdminActions::Ban: failed to persist ban for {}.", steamId);
        return BanResult::PersistFailed;
    }

    Log::Info("IAdminActions::Ban: banned {} ({}s): {}", steamId, durationSec, ban.Reason);
    return BanResult::Ok;
}

void AdminActionsService::AlertAdmins(int64_t steamId, std::string_view detector, int score)
{
    auto* suspect = _rt.Players.BySteamId(steamId);
    const std::string suspectName = suspect ? suspect->Name() : std::to_string(steamId);
    const std::string detectorName(detector);
    const std::string scoreText = std::to_string(score);

    for (auto* admin : _rt.Players.All())
    {
        if (!_access.HasPermission(admin->SteamId(), Permission::Ban))
            continue;
        _rt.Messages.ReplyKey(admin->Slot(), "anticheat.alert",
                              {{"name", suspectName}, {"detector", detectorName}, {"score", scoreText}});
    }
}

}  // namespace AdminSystem::Core
