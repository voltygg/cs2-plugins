#include "AdminActionsService.hpp"

#include "../Database/Entities/Ban.hpp"
#include "App.hpp"
#include "Permissions.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Log.hpp>
#include <CS2Kit/Core/SteamId.hpp>
#include <string>

namespace Log = CS2Kit::Core::Log;
using Contracts::BanResult;

namespace AdminSystem::Core
{

void AdminActionsService::Publish()
{
    _app.Runtime.Exchange.Publish<Contracts::IAdminActions>(this);
}

void AdminActionsService::Unpublish()
{
    _app.Runtime.Exchange.Unpublish<Contracts::IAdminActions>();
}

BanResult AdminActionsService::Ban(int64_t steamId, int64_t durationSec, std::string_view reason)
{
    if (!CS2Kit::Core::SteamId::IsValid(steamId))
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
    if (auto* target = _app.Runtime.Players.GetPlayerBySteamId(steamId))
    {
        ban.TargetName = target->GetName();
        ban.TargetIp = target->GetIpAddress();
    }

    if (!_app.Punishments.IssueBan(ban))
    {
        Log::Warn("IAdminActions::Ban: failed to persist ban for {}.", steamId);
        return BanResult::PersistFailed;
    }

    Log::Info("IAdminActions::Ban: banned {} ({}s): {}", steamId, durationSec, ban.Reason);
    return BanResult::Ok;
}

void AdminActionsService::AlertAdmins(int64_t steamId, std::string_view detector, int score)
{
    auto* suspect = _app.Runtime.Players.GetPlayerBySteamId(steamId);
    const std::string suspectName = suspect ? suspect->GetName() : std::to_string(steamId);
    const std::string detectorName(detector);
    const std::string scoreText = std::to_string(score);

    for (auto* admin : _app.Runtime.Players.GetAllPlayers())
    {
        if (!_app.Admins.HasPermission(admin->GetSteamID(), Permission::Ban))
            continue;
        _app.Runtime.Messages.ReplyKey(admin->GetSlot(), "anticheat.alert",
                                       {{"name", suspectName}, {"detector", detectorName}, {"score", scoreText}});
    }
}

}  // namespace AdminSystem::Core
