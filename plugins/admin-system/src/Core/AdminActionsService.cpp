#include "AdminActionsService.hpp"

#include "../Database/Entities/Ban.hpp"
#include "Managers.hpp"
#include "Permissions.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <CS2Kit/Utils/SteamId.hpp>
#include <string>

namespace Log = CS2Kit::Utils::Log;
using Contracts::BanResult;
using CS2Kit::Core::Engine;

namespace AdminSystem::Core
{

void AdminActionsService::Publish()
{
    Engine().Exchange.Publish<Contracts::IAdminActions>(this);
}

void AdminActionsService::Unpublish()
{
    Engine().Exchange.Unpublish<Contracts::IAdminActions>();
}

BanResult AdminActionsService::Ban(int64_t steamId, int64_t durationSec, std::string_view reason)
{
    if (!CS2Kit::Utils::SteamId::IsValid(steamId))
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
    if (auto* target = Engine().Players.GetPlayerBySteamId(steamId))
    {
        ban.TargetName = target->GetName();
        ban.TargetIp = target->GetIpAddress();
    }

    if (!App().Punishments.IssueBan(ban))
    {
        Log::Warn("IAdminActions::Ban: failed to persist ban for {}.", steamId);
        return BanResult::PersistFailed;
    }

    Log::Info("IAdminActions::Ban: banned {} ({}s): {}", steamId, durationSec, ban.Reason);
    return BanResult::Ok;
}

void AdminActionsService::AlertAdmins(int64_t steamId, std::string_view detector, int score)
{
    auto* suspect = Engine().Players.GetPlayerBySteamId(steamId);
    const std::string suspectName = suspect ? suspect->GetName() : std::to_string(steamId);
    const std::string detectorName(detector);
    const std::string scoreText = std::to_string(score);

    for (auto* admin : Engine().Players.GetAllPlayers())
    {
        if (!App().Admins.HasPermission(admin->GetSteamID(), Permission::Ban))
            continue;
        Engine().Messages.ReplyKey(admin->GetSlot(), "anticheat.alert",
                                   {{"name", suspectName}, {"detector", detectorName}, {"score", scoreText}});
    }
}

}  // namespace AdminSystem::Core
