#include "PunishmentManager.hpp"
#include "../Core/Managers.hpp"
#include <CS2Kit/Core/Services.hpp>

#include "../Core/ChatService.hpp"
#include "../Core/Config.hpp"
#include "../Database/Repositories/BanRepository.hpp"
#include "../Database/Repositories/MuteRepository.hpp"
#include "../Database/Repositories/WarningRepository.hpp"

#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/GameInterfaces.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>

using CS2Kit::Core::Kit;

namespace AdminSystem::Punishments
{

using namespace AdminSystem::Database;
using namespace CS2Kit::Players;
using namespace CS2Kit::Utils;
using AdminSystem::Core::ChatService;
using AdminSystem::Core::ConfigManager;

namespace
{

// Push the engine's per-pair voice channel state for `senderSteamId` to all currently
// connected listeners. Our SetClientListening hook will still enforce the cached
// IsVoiceMuted() decision; this just forces the engine to re-evaluate any channels
// that were already negotiated before the (un)mute landed.
void RefreshVoiceChannel(int64_t senderSteamId, bool muted)
{
    auto* engine = Kit().Interfaces.Engine;
    if (!engine)
        return;

    auto* sender = Kit().Players.GetPlayerBySteamId(senderSteamId);
    if (!sender)
        return;

    int senderSlot = sender->GetSlot();
    for (int i = 0; i < 64; ++i)
    {
        if (i == senderSlot)
            continue;
        if (!Kit().Players.GetPlayerBySlot(i))
            continue;
        engine->SetClientListening(CPlayerSlot(i), CPlayerSlot(senderSlot), !muted);
    }
}

}  // namespace

bool PunishmentManager::LoadActivePunishments()
{
    try
    {
        BanRepository banRepo;
        MuteRepository<VoiceMute> voiceRepo{"voice_mutes", "voice_mute"};
        MuteRepository<TextMute> textRepo{"text_mutes", "text_mute"};

        _activeBans.clear();
        for (const auto& ban : banRepo.FindAllActive())
            _activeBans[ban.TargetSteamId] = ban;

        _activeVoiceMutes.clear();
        _voiceMutedPlayers.clear();
        for (const auto& mute : voiceRepo.FindAllActive())
        {
            _activeVoiceMutes[mute.TargetSteamId] = mute;
            _voiceMutedPlayers.insert(mute.TargetSteamId);
        }

        _activeTextMutes.clear();
        _textMutedPlayers.clear();
        for (const auto& mute : textRepo.FindAllActive())
        {
            _activeTextMutes[mute.TargetSteamId] = mute;
            _textMutedPlayers.insert(mute.TargetSteamId);
        }

        Log::Info("Loaded active punishments: {} ban(s), {} voice mute(s), {} text mute(s).", _activeBans.size(),
                  _activeVoiceMutes.size(), _activeTextMutes.size());
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

std::optional<VoiceMute> PunishmentManager::GetActiveVoiceMute(int64_t steamId)
{
    auto it = _activeVoiceMutes.find(steamId);
    if (it == _activeVoiceMutes.end())
        return std::nullopt;
    if (it->second.IsExpired())
    {
        _activeVoiceMutes.erase(it);
        _voiceMutedPlayers.erase(steamId);
        RefreshVoiceChannel(steamId, false);
        return std::nullopt;
    }
    return it->second;
}

std::optional<TextMute> PunishmentManager::GetActiveTextMute(int64_t steamId)
{
    auto it = _activeTextMutes.find(steamId);
    if (it == _activeTextMutes.end())
        return std::nullopt;
    if (it->second.IsExpired())
    {
        _activeTextMutes.erase(it);
        _textMutedPlayers.erase(steamId);
        return std::nullopt;
    }
    return it->second;
}

bool PunishmentManager::IsVoiceMuted(int64_t steamId)
{
    return _voiceMutedPlayers.count(steamId) > 0;
}

bool PunishmentManager::IsTextMuted(int64_t steamId)
{
    return _textMutedPlayers.count(steamId) > 0;
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
        if (auto* player = Kit().Players.GetPlayerBySteamId(ban.TargetSteamId))
        {
            CS2Kit::Sdk::PlayerController controller(player->GetSlot());
            controller.Kick(ban.Reason.c_str());
        }

        Sys().Chat.BroadcastPunishment("banned", ban.AdminName, ban.TargetName, ban.Reason, ban.Duration);
        return true;
    }
    catch (const std::exception& e)
    {
        Log::Warn("IssueBan exception: {}", e.what());
        return false;
    }
}

bool PunishmentManager::IssueVoiceMute(VoiceMute& mute)
{
    try
    {
        if (mute.CreatedAt == 0)
            mute.CreatedAt = TimeUtils::Now();
        if (mute.Duration > 0 && mute.ExpiresAt == 0)
            mute.ExpiresAt = TimeUtils::GetExpirationTime(mute.Duration);

        MuteRepository<VoiceMute> repo{"voice_mutes", "voice_mute"};
        if (!repo.Create(mute))
            return false;

        _activeVoiceMutes[mute.TargetSteamId] = mute;
        _voiceMutedPlayers.insert(mute.TargetSteamId);
        RefreshVoiceChannel(mute.TargetSteamId, true);

        Sys().Chat.BroadcastPunishment("voice-muted", mute.AdminName, mute.TargetName, mute.Reason,
                                                    mute.Duration);
        return true;
    }
    catch (const std::exception& e)
    {
        Log::Warn("IssueVoiceMute exception: {}", e.what());
        return false;
    }
}

bool PunishmentManager::IssueTextMute(TextMute& mute)
{
    try
    {
        if (mute.CreatedAt == 0)
            mute.CreatedAt = TimeUtils::Now();
        if (mute.Duration > 0 && mute.ExpiresAt == 0)
            mute.ExpiresAt = TimeUtils::GetExpirationTime(mute.Duration);

        MuteRepository<TextMute> repo{"text_mutes", "text_mute"};
        if (!repo.Create(mute))
            return false;

        _activeTextMutes[mute.TargetSteamId] = mute;
        _textMutedPlayers.insert(mute.TargetSteamId);

        Sys().Chat.BroadcastPunishment("text-muted", mute.AdminName, mute.TargetName, mute.Reason,
                                                    mute.Duration);
        return true;
    }
    catch (const std::exception& e)
    {
        Log::Warn("IssueTextMute exception: {}", e.what());
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

        Sys().Chat.BroadcastPunishment("warned", warning.AdminName, warning.TargetName, warning.Reason, 0);

        int active = repo.CountActive(warning.TargetSteamId);
        int threshold = Sys().Config.GetPunishments().warningThreshold;
        if (threshold > 0 && active >= threshold)
        {
            Log::Info("Warning threshold ({}) reached for {} -- escalating to ban.", threshold, warning.TargetSteamId);
            repo.Clear(warning.TargetSteamId);

            Ban autoBan;
            autoBan.TargetSteamId = warning.TargetSteamId;
            autoBan.TargetName = warning.TargetName;
            autoBan.AdminSteamId = warning.AdminSteamId;
            autoBan.AdminName = warning.AdminName;
            autoBan.Reason = Sys().Config.GetPunishments().defaultBanReason;
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
            Sys().Chat.BroadcastPunishment("unbanned", "Admin", it->second.TargetName, reason, 0);
            _activeBans.erase(it);
            return true;
        }
    }
    return true;
}

bool PunishmentManager::RemoveVoiceMute(int64_t muteId, int64_t removedBy, const std::string& reason)
{
    MuteRepository<VoiceMute> repo{"voice_mutes", "voice_mute"};
    if (!repo.Remove(muteId, removedBy, reason))
        return false;

    for (auto it = _activeVoiceMutes.begin(); it != _activeVoiceMutes.end(); ++it)
    {
        if (it->second.Id == muteId)
        {
            int64_t target = it->first;
            std::string targetName = it->second.TargetName;
            _voiceMutedPlayers.erase(target);
            _activeVoiceMutes.erase(it);
            RefreshVoiceChannel(target, false);
            Sys().Chat.BroadcastPunishment("voice-unmuted", "Admin", targetName, reason, 0);
            return true;
        }
    }
    return true;
}

bool PunishmentManager::RemoveTextMute(int64_t muteId, int64_t removedBy, const std::string& reason)
{
    MuteRepository<TextMute> repo{"text_mutes", "text_mute"};
    if (!repo.Remove(muteId, removedBy, reason))
        return false;

    for (auto it = _activeTextMutes.begin(); it != _activeTextMutes.end(); ++it)
    {
        if (it->second.Id == muteId)
        {
            _textMutedPlayers.erase(it->first);
            Sys().Chat.BroadcastPunishment("text-unmuted", "Admin", it->second.TargetName, reason, 0);
            _activeTextMutes.erase(it);
            return true;
        }
    }
    return true;
}

bool PunishmentManager::RemoveBanBySteamId(int64_t steamId, int64_t removedBy, const std::string& reason)
{
    BanRepository repo;
    return RemoveBySteamIdImpl(_activeBans, repo, steamId, removedBy, reason, &PunishmentManager::RemoveBan);
}

bool PunishmentManager::RemoveVoiceMuteBySteamId(int64_t steamId, int64_t removedBy, const std::string& reason)
{
    MuteRepository<VoiceMute> repo{"voice_mutes", "voice_mute"};
    return RemoveBySteamIdImpl(_activeVoiceMutes, repo, steamId, removedBy, reason,
                               &PunishmentManager::RemoveVoiceMute);
}

bool PunishmentManager::RemoveTextMuteBySteamId(int64_t steamId, int64_t removedBy, const std::string& reason)
{
    MuteRepository<TextMute> repo{"text_mutes", "text_mute"};
    return RemoveBySteamIdImpl(_activeTextMutes, repo, steamId, removedBy, reason, &PunishmentManager::RemoveTextMute);
}

void PunishmentManager::ExpireOldPunishments()
{
    try
    {
        BanRepository banRepo;
        MuteRepository<VoiceMute> voiceRepo{"voice_mutes", "voice_mute"};
        MuteRepository<TextMute> textRepo{"text_mutes", "text_mute"};

        banRepo.ExpireOldBans();
        voiceRepo.ExpireOld();
        textRepo.ExpireOld();

        // Snapshot the muted set before reload so we can detect voice mutes that
        // expired this sweep and refresh their voice channels.
        auto previouslyMuted = _voiceMutedPlayers;

        // Cheaper than tracking individual expirations: just rebuild caches from the DB.
        LoadActivePunishments();

        for (int64_t steamId : previouslyMuted)
        {
            if (!_voiceMutedPlayers.count(steamId))
                RefreshVoiceChannel(steamId, false);
        }
    }
    catch (const std::exception& e)
    {
        Log::Warn("ExpireOldPunishments exception: {}", e.what());
    }
}

}  // namespace AdminSystem::Punishments
