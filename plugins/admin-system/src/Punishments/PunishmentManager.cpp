#include "PunishmentManager.hpp"

#include "../Core/ChatService.hpp"
#include "../Core/Config.hpp"
#include "../Core/Managers.hpp"
#include "../Database/Repositories/BanRepository.hpp"
#include "../Database/Repositories/MuteRepository.hpp"
#include "../Database/Repositories/WarningRepository.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/GameInterfaces.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>
#include <algorithm>
#include <utility>

using CS2Kit::Core::Engine;

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
    auto* engine = Engine().Interfaces.Engine;
    if (!engine)
        return;

    auto* sender = Engine().Players.GetPlayerBySteamId(senderSteamId);
    if (!sender)
        return;

    int senderSlot = sender->GetSlot();
    for (int i = 0; i < 64; ++i)
    {
        if (i == senderSlot)
            continue;
        if (!Engine().Players.GetPlayerBySlot(i))
            continue;
        engine->SetClientListening(CPlayerSlot(i), CPlayerSlot(senderSlot), !muted);
    }
}

void StampTimes(auto& entity)
{
    if (entity.CreatedAt == 0)
        entity.CreatedAt = TimeUtils::Now();
    if (entity.Duration > 0 && entity.ExpiresAt == 0)
        entity.ExpiresAt = TimeUtils::GetExpirationTime(entity.Duration);
}

}  // namespace

bool PunishmentManager::LoadActivePunishments()
{
    _activeBans.clear();
    for (const auto& ban : BanRepository{}.FindAllActive())
        _activeBans[ban.TargetSteamId] = ban;

    _activeVoiceMutes.clear();
    _voiceMutedPlayers.clear();
    for (const auto& mute : MuteRepository<VoiceMute>{}.FindAllActive())
    {
        _activeVoiceMutes[mute.TargetSteamId] = mute;
        _voiceMutedPlayers.insert(mute.TargetSteamId);
    }

    _activeTextMutes.clear();
    _textMutedPlayers.clear();
    for (const auto& mute : MuteRepository<TextMute>{}.FindAllActive())
    {
        _activeTextMutes[mute.TargetSteamId] = mute;
        _textMutedPlayers.insert(mute.TargetSteamId);
    }

    Log::Info("Loaded active punishments: {} ban(s), {} voice mute(s), {} text mute(s).", _activeBans.size(),
              _activeVoiceMutes.size(), _activeTextMutes.size());
    return true;
}

std::vector<Ban> PunishmentManager::GetActiveBans() const
{
    return SnapshotActive(_activeBans);
}

std::vector<VoiceMute> PunishmentManager::GetActiveVoiceMutes() const
{
    return SnapshotActive(_activeVoiceMutes);
}

std::vector<TextMute> PunishmentManager::GetActiveTextMutes() const
{
    return SnapshotActive(_activeTextMutes);
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
    StampTimes(ban);
    _activeBans[ban.TargetSteamId] = ban;

    // The insert rides the worker; the generated row id lands in the cache when it returns
    // (the unban menu snapshots the cache, so the id is there by the time a human clicks).
    BanRepository{}.CreateAsync(ban, [this, steamId = ban.TargetSteamId](int64_t id) {
        if (auto it = _activeBans.find(steamId); it != _activeBans.end() && it->second.Id == 0)
            it->second.Id = id;
    });

    // Kick the player if currently connected.
    if (auto* player = Engine().Players.GetPlayerBySteamId(ban.TargetSteamId))
        player->Controller().Kick(ban.Reason.c_str());

    App().Chat.BroadcastPunishment("banned", ban.AdminName, ban.TargetName, ban.Reason, ban.Duration);
    return true;
}

bool PunishmentManager::IssueVoiceMute(VoiceMute& mute)
{
    StampTimes(mute);
    _activeVoiceMutes[mute.TargetSteamId] = mute;
    _voiceMutedPlayers.insert(mute.TargetSteamId);
    RefreshVoiceChannel(mute.TargetSteamId, true);

    MuteRepository<VoiceMute>{}.CreateAsync(mute, [this, steamId = mute.TargetSteamId](int64_t id) {
        if (auto it = _activeVoiceMutes.find(steamId); it != _activeVoiceMutes.end() && it->second.Id == 0)
            it->second.Id = id;
    });

    App().Chat.BroadcastPunishment("voice-muted", mute.AdminName, mute.TargetName, mute.Reason, mute.Duration);
    return true;
}

bool PunishmentManager::IssueTextMute(TextMute& mute)
{
    StampTimes(mute);
    _activeTextMutes[mute.TargetSteamId] = mute;
    _textMutedPlayers.insert(mute.TargetSteamId);

    MuteRepository<TextMute>{}.CreateAsync(mute, [this, steamId = mute.TargetSteamId](int64_t id) {
        if (auto it = _activeTextMutes.find(steamId); it != _activeTextMutes.end() && it->second.Id == 0)
            it->second.Id = id;
    });

    App().Chat.BroadcastPunishment("text-muted", mute.AdminName, mute.TargetName, mute.Reason, mute.Duration);
    return true;
}

bool PunishmentManager::IssueWarning(Warning& warning)
{
    if (warning.CreatedAt == 0)
        warning.CreatedAt = TimeUtils::Now();

    WarningRepository repo;
    repo.CreateAsync(warning);
    App().Chat.BroadcastPunishment("warned", warning.AdminName, warning.TargetName, warning.Reason, 0);

    // Jobs run FIFO on the worker, so this count sees the insert above. Escalation happens on
    // the game thread when the count arrives.
    repo.CountActiveAsync(warning.TargetSteamId, [this, w = warning](int active) {
        int threshold = App().Config.GetPunishments().warningThreshold;
        if (threshold <= 0 || active < threshold)
            return;

        Log::Info("Warning threshold ({}) reached for {} -- escalating to ban.", threshold, w.TargetSteamId);
        WarningRepository{}.ClearAsync(w.TargetSteamId);

        Ban autoBan;
        autoBan.TargetSteamId = w.TargetSteamId;
        autoBan.TargetName = w.TargetName;
        autoBan.AdminSteamId = w.AdminSteamId;
        autoBan.AdminName = w.AdminName;
        autoBan.Reason = App().Config.GetPunishments().defaultBanReason;
        autoBan.Duration = 0;  // permanent escalation
        IssueBan(autoBan);
    });

    return true;
}

bool PunishmentManager::RemoveBan(int64_t banId, int64_t removedBy, const std::string& reason)
{
    for (auto it = _activeBans.begin(); it != _activeBans.end(); ++it)
    {
        if (it->second.Id == banId)
        {
            BanRepository{}.RemoveAsync(banId, removedBy, reason);
            App().Chat.BroadcastPunishment("unbanned", "Admin", it->second.TargetName, reason, 0);
            _activeBans.erase(it);
            return true;
        }
    }
    return false;  // already lifted (possibly by another server) - nothing to remove
}

bool PunishmentManager::RemoveVoiceMute(int64_t muteId, int64_t removedBy, const std::string& reason)
{
    for (auto it = _activeVoiceMutes.begin(); it != _activeVoiceMutes.end(); ++it)
    {
        if (it->second.Id == muteId)
        {
            int64_t target = it->first;
            std::string targetName = it->second.TargetName;
            MuteRepository<VoiceMute>{}.RemoveAsync(muteId, removedBy, reason);
            _voiceMutedPlayers.erase(target);
            _activeVoiceMutes.erase(it);
            RefreshVoiceChannel(target, false);
            App().Chat.BroadcastPunishment("voice-unmuted", "Admin", targetName, reason, 0);
            return true;
        }
    }
    return false;
}

bool PunishmentManager::RemoveTextMute(int64_t muteId, int64_t removedBy, const std::string& reason)
{
    for (auto it = _activeTextMutes.begin(); it != _activeTextMutes.end(); ++it)
    {
        if (it->second.Id == muteId)
        {
            MuteRepository<TextMute>{}.RemoveAsync(muteId, removedBy, reason);
            _textMutedPlayers.erase(it->first);
            App().Chat.BroadcastPunishment("text-unmuted", "Admin", it->second.TargetName, reason, 0);
            _activeTextMutes.erase(it);
            return true;
        }
    }
    return false;
}

bool PunishmentManager::RemoveBanBySteamId(int64_t steamId, int64_t removedBy, const std::string& reason)
{
    return RemoveBySteamIdImpl(_activeBans, steamId, removedBy, reason, &PunishmentManager::RemoveBan);
}

bool PunishmentManager::RemoveVoiceMuteBySteamId(int64_t steamId, int64_t removedBy, const std::string& reason)
{
    return RemoveBySteamIdImpl(_activeVoiceMutes, steamId, removedBy, reason, &PunishmentManager::RemoveVoiceMute);
}

bool PunishmentManager::RemoveTextMuteBySteamId(int64_t steamId, int64_t removedBy, const std::string& reason)
{
    return RemoveBySteamIdImpl(_activeTextMutes, steamId, removedBy, reason, &PunishmentManager::RemoveTextMute);
}

void PunishmentManager::ExpireOldPunishments()
{
    BanRepository{}.ExpireOldAsync();
    MuteRepository<VoiceMute>{}.ExpireOldAsync();
    MuteRepository<TextMute>{}.ExpireOldAsync();

    // FIFO: these snapshots run after the expirations above have landed.
    RefreshCachesAsync();
}

void PunishmentManager::RefreshCachesAsync()
{
    BanRepository{}.FindAllActiveAsync([this](std::vector<Ban> bans) {
        _activeBans.clear();
        for (auto& ban : bans)
            _activeBans[ban.TargetSteamId] = std::move(ban);
    });

    MuteRepository<VoiceMute>{}.FindAllActiveAsync([this](std::vector<VoiceMute> mutes) {
        // Snapshot the muted set before the swap so voice mutes that expired this sweep get
        // their voice channels refreshed.
        auto previouslyMuted = _voiceMutedPlayers;

        _activeVoiceMutes.clear();
        _voiceMutedPlayers.clear();
        for (auto& mute : mutes)
        {
            _voiceMutedPlayers.insert(mute.TargetSteamId);
            _activeVoiceMutes[mute.TargetSteamId] = std::move(mute);
        }

        for (int64_t steamId : previouslyMuted)
        {
            if (!_voiceMutedPlayers.count(steamId))
                RefreshVoiceChannel(steamId, false);
        }
    });

    MuteRepository<TextMute>{}.FindAllActiveAsync([this](std::vector<TextMute> mutes) {
        _activeTextMutes.clear();
        _textMutedPlayers.clear();
        for (auto& mute : mutes)
        {
            _textMutedPlayers.insert(mute.TargetSteamId);
            _activeTextMutes[mute.TargetSteamId] = std::move(mute);
        }
    });
}

}  // namespace AdminSystem::Punishments
