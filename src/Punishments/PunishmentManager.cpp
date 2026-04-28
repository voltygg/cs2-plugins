#include "PunishmentManager.hpp"

#include "../Core/ChatService.hpp"
#include "../Core/Config.hpp"
#include "../Database/Repositories/BanRepository.hpp"
#include "../Database/Repositories/GagRepository.hpp"
#include "../Database/Repositories/MuteRepository.hpp"
#include "../Database/Repositories/WarningRepository.hpp"

#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>

namespace AdminSystem::Punishments
{

using namespace AdminSystem::Database;
using namespace CS2Kit::Players;
using namespace CS2Kit::Utils;
using AdminSystem::Core::ChatService;
using AdminSystem::Core::ConfigManager;

bool PunishmentManager::LoadActivePunishments()
{
    try
    {
        BanRepository banRepo;
        MuteRepository muteRepo;
        GagRepository gagRepo;

        _activeBans.clear();
        for (const auto& ban : banRepo.FindAllActive())
            _activeBans[ban.TargetSteamId] = ban;

        _activeMutes.clear();
        _mutedPlayers.clear();
        for (const auto& mute : muteRepo.FindAllActive())
        {
            _activeMutes[mute.TargetSteamId] = mute;
            _mutedPlayers.insert(mute.TargetSteamId);
        }

        _activeGags.clear();
        _gaggedPlayers.clear();
        for (const auto& gag : gagRepo.FindAllActive())
        {
            _activeGags[gag.TargetSteamId] = gag;
            _gaggedPlayers.insert(gag.TargetSteamId);
        }

        Log::Info("Loaded active punishments: {} ban(s), {} mute(s), {} gag(s).", _activeBans.size(),
                  _activeMutes.size(), _activeGags.size());
        return true;
    }
    catch (const std::exception& e)
    {
        Log::Warn("LoadActivePunishments failed: {}", e.what());
        return false;
    }
}

std::optional<Ban> PunishmentManager::GetActiveBan(int64_t steamId)
{
    auto it = _activeBans.find(steamId);
    if (it == _activeBans.end())
        return std::nullopt;
    if (it->second.IsExpired())
    {
        _activeBans.erase(it);
        return std::nullopt;
    }
    return it->second;
}

std::optional<Mute> PunishmentManager::GetActiveMute(int64_t steamId)
{
    auto it = _activeMutes.find(steamId);
    if (it == _activeMutes.end())
        return std::nullopt;
    if (it->second.IsExpired())
    {
        _activeMutes.erase(it);
        _mutedPlayers.erase(steamId);
        return std::nullopt;
    }
    return it->second;
}

std::optional<Gag> PunishmentManager::GetActiveGag(int64_t steamId)
{
    auto it = _activeGags.find(steamId);
    if (it == _activeGags.end())
        return std::nullopt;
    if (it->second.IsExpired())
    {
        _activeGags.erase(it);
        _gaggedPlayers.erase(steamId);
        return std::nullopt;
    }
    return it->second;
}

bool PunishmentManager::IsMuted(int64_t steamId)
{
    return _mutedPlayers.count(steamId) > 0;
}

bool PunishmentManager::IsGagged(int64_t steamId)
{
    return _gaggedPlayers.count(steamId) > 0;
}

bool PunishmentManager::IssueBan(Ban& ban)
{
    try
    {
        if (ban.CreatedAt == 0)
            ban.CreatedAt = TimeUtils::Now();
        if (ban.Duration > 0 && ban.ExpiresAt == 0)
            ban.ExpiresAt = TimeUtils::GetExpirationTime(ban.Duration);

        BanRepository repo;
        if (!repo.Create(ban))
        {
            Log::Warn("Failed to persist ban for {}", ban.TargetSteamId);
            return false;
        }

        _activeBans[ban.TargetSteamId] = ban;

        // Kick the player if currently connected.
        if (auto* player = PlayerManager::Instance().GetPlayerBySteamId(ban.TargetSteamId))
        {
            CS2Kit::Sdk::PlayerController controller(player->GetSlot());
            controller.Kick(ban.Reason.c_str());
        }

        ChatService::Instance().BroadcastPunishment("banned", ban.AdminName, ban.TargetName, ban.Reason, ban.Duration);
        return true;
    }
    catch (const std::exception& e)
    {
        Log::Warn("IssueBan exception: {}", e.what());
        return false;
    }
}

bool PunishmentManager::IssueMute(Mute& mute)
{
    try
    {
        if (mute.CreatedAt == 0)
            mute.CreatedAt = TimeUtils::Now();
        if (mute.Duration > 0 && mute.ExpiresAt == 0)
            mute.ExpiresAt = TimeUtils::GetExpirationTime(mute.Duration);

        MuteRepository repo;
        if (!repo.Create(mute))
            return false;

        _activeMutes[mute.TargetSteamId] = mute;
        _mutedPlayers.insert(mute.TargetSteamId);

        // TODO(voice-block): CS2Kit doesn't expose a SVC_VoiceData hook today, so the mute is
        // persisted and visible in `!who` but voice still flows. Plug into voice transport when
        // CS2Kit gains that surface.
        ChatService::Instance().BroadcastPunishment("muted", mute.AdminName, mute.TargetName, mute.Reason,
                                                    mute.Duration);
        return true;
    }
    catch (const std::exception& e)
    {
        Log::Warn("IssueMute exception: {}", e.what());
        return false;
    }
}

bool PunishmentManager::IssueGag(Gag& gag)
{
    try
    {
        if (gag.CreatedAt == 0)
            gag.CreatedAt = TimeUtils::Now();
        if (gag.Duration > 0 && gag.ExpiresAt == 0)
            gag.ExpiresAt = TimeUtils::GetExpirationTime(gag.Duration);

        GagRepository repo;
        if (!repo.Create(gag))
            return false;

        _activeGags[gag.TargetSteamId] = gag;
        _gaggedPlayers.insert(gag.TargetSteamId);

        ChatService::Instance().BroadcastPunishment("gagged", gag.AdminName, gag.TargetName, gag.Reason, gag.Duration);
        return true;
    }
    catch (const std::exception& e)
    {
        Log::Warn("IssueGag exception: {}", e.what());
        return false;
    }
}

bool PunishmentManager::IssueWarning(Warning& warning)
{
    try
    {
        if (warning.CreatedAt == 0)
            warning.CreatedAt = TimeUtils::Now();

        WarningRepository repo;
        if (!repo.Create(warning))
            return false;

        ChatService::Instance().BroadcastPunishment("warned", warning.AdminName, warning.TargetName, warning.Reason, 0);

        int active = repo.CountActive(warning.TargetSteamId);
        int threshold = ConfigManager::Instance().GetPluginConfig().MaxWarnings;
        if (threshold > 0 && active >= threshold)
        {
            Log::Info("Warning threshold ({}) reached for {} -- escalating to ban.", threshold, warning.TargetSteamId);
            repo.Clear(warning.TargetSteamId);

            Ban autoBan;
            autoBan.TargetSteamId = warning.TargetSteamId;
            autoBan.TargetName = warning.TargetName;
            autoBan.AdminSteamId = warning.AdminSteamId;
            autoBan.AdminName = warning.AdminName;
            autoBan.Reason = ConfigManager::Instance().GetPluginConfig().DefaultBanReason;
            autoBan.Duration = 0;  // permanent escalation
            IssueBan(autoBan);
        }
        return true;
    }
    catch (const std::exception& e)
    {
        Log::Warn("IssueWarning exception: {}", e.what());
        return false;
    }
}

bool PunishmentManager::RemoveBan(int64_t banId, int64_t removedBy, const std::string& reason)
{
    BanRepository repo;
    if (!repo.Remove(banId, removedBy, reason))
        return false;

    for (auto it = _activeBans.begin(); it != _activeBans.end(); ++it)
    {
        if (it->second.Id == banId)
        {
            ChatService::Instance().BroadcastPunishment("unbanned", "Admin", it->second.TargetName, reason, 0);
            _activeBans.erase(it);
            return true;
        }
    }
    return true;
}

bool PunishmentManager::RemoveMute(int64_t muteId, int64_t removedBy, const std::string& reason)
{
    MuteRepository repo;
    if (!repo.Remove(muteId, removedBy, reason))
        return false;

    for (auto it = _activeMutes.begin(); it != _activeMutes.end(); ++it)
    {
        if (it->second.Id == muteId)
        {
            _mutedPlayers.erase(it->first);
            ChatService::Instance().BroadcastPunishment("unmuted", "Admin", it->second.TargetName, reason, 0);
            _activeMutes.erase(it);
            return true;
        }
    }
    return true;
}

bool PunishmentManager::RemoveGag(int64_t gagId, int64_t removedBy, const std::string& reason)
{
    GagRepository repo;
    if (!repo.Remove(gagId, removedBy, reason))
        return false;

    for (auto it = _activeGags.begin(); it != _activeGags.end(); ++it)
    {
        if (it->second.Id == gagId)
        {
            _gaggedPlayers.erase(it->first);
            ChatService::Instance().BroadcastPunishment("ungagged", "Admin", it->second.TargetName, reason, 0);
            _activeGags.erase(it);
            return true;
        }
    }
    return true;
}

bool PunishmentManager::RemoveBanBySteamId(int64_t steamId, int64_t removedBy, const std::string& reason)
{
    auto it = _activeBans.find(steamId);
    if (it != _activeBans.end())
        return RemoveBan(it->second.Id, removedBy, reason);

    // Fall back to a DB lookup so we can unban offline / never-cached players.
    BanRepository repo;
    if (auto found = repo.FindActiveBySteamId(steamId))
        return RemoveBan(found->Id, removedBy, reason);
    return false;
}

bool PunishmentManager::RemoveMuteBySteamId(int64_t steamId, int64_t removedBy, const std::string& reason)
{
    auto it = _activeMutes.find(steamId);
    if (it != _activeMutes.end())
        return RemoveMute(it->second.Id, removedBy, reason);

    MuteRepository repo;
    if (auto found = repo.FindActiveBySteamId(steamId))
        return RemoveMute(found->Id, removedBy, reason);
    return false;
}

bool PunishmentManager::RemoveGagBySteamId(int64_t steamId, int64_t removedBy, const std::string& reason)
{
    auto it = _activeGags.find(steamId);
    if (it != _activeGags.end())
        return RemoveGag(it->second.Id, removedBy, reason);

    GagRepository repo;
    if (auto found = repo.FindActiveBySteamId(steamId))
        return RemoveGag(found->Id, removedBy, reason);
    return false;
}

void PunishmentManager::ExpireOldPunishments()
{
    try
    {
        BanRepository banRepo;
        MuteRepository muteRepo;
        GagRepository gagRepo;

        banRepo.ExpireOldBans();
        muteRepo.ExpireOldMutes();
        gagRepo.ExpireOldGags();

        // Cheaper than tracking individual expirations: just rebuild caches from the DB.
        LoadActivePunishments();
    }
    catch (const std::exception& e)
    {
        Log::Warn("ExpireOldPunishments exception: {}", e.what());
    }
}

}  // namespace AdminSystem::Punishments
