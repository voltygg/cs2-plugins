#include "PunishmentManager.hpp"

#include "../Config/ConfigManager.hpp"
#include "../Core/ChatService.hpp"
#include "KickNotice.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Time.hpp>
#include <VoltMod/Engine/Interfaces.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <algorithm>
#include <utility>

using AdminSystem::Database::Punishment;
using VoltMod::Time;

namespace AdminSystem::Punishments
{

namespace Log = VoltMod::Log;

// The caches are indexed by PunishType, so the array has to cover the whole enum.
static_assert(static_cast<int>(PunishType::Warn) == 4, "PunishmentManager::_active has one slot per PunishType");

/** The broadcast key for issuing @p kind. */
static std::string_view IssuedKey(PunishType kind)
{
    switch (kind)
    {
    case PunishType::Ban:
        return "banned";
    case PunishType::VoiceMute:
        return "voice-muted";
    case PunishType::TextMute:
        return "text-muted";
    case PunishType::Warn:
        return "warned";
    case PunishType::Kick:
        break;
    }
    return "kicked";
}

/** The broadcast key for lifting @p kind. Only the cached kinds are ever lifted. */
static std::string_view LiftedKey(PunishType kind)
{
    switch (kind)
    {
    case PunishType::VoiceMute:
        return "voice-unmuted";
    case PunishType::TextMute:
        return "text-unmuted";
    case PunishType::Ban:
    case PunishType::Warn:
    case PunishType::Kick:
        break;
    }
    return "unbanned";
}

/** Fill in the times an admin did not give. */
static void StampTimes(Punishment& record)
{
    if (record.CreatedAt == 0)
        record.CreatedAt = Time::Now();
    if (record.Duration > 0 && record.ExpiresAt == 0)
        record.ExpiresAt = Time::GetExpirationTime(record.Duration);
}

// Make the engine re-evaluate channels negotiated before the (un)mute landed. The
// SetClientListening hook still enforces the cached decision.
static void RefreshVoiceChannel(VoltMod::Runtime& rt, int64_t senderSteamId, bool muted)
{
    auto* engine = rt.Unsafe.Interfaces.Engine;
    if (!engine)
        return;

    auto* sender = rt.Players.BySteamId(senderSteamId);
    if (!sender)
        return;

    int senderSlot = sender->Slot();
    for (int i = 0; i < 64; ++i)
    {
        if (i == senderSlot)
            continue;
        if (!rt.Players.Get(i))
            continue;
        engine->SetClientListening(CPlayerSlot(i), CPlayerSlot(senderSlot), !muted);
    }
}

bool PunishmentManager::LoadActivePunishments()
{
    SwapCaches(_repos.Punishments.FindAllActive());
    Log::Info("Loaded active punishments: {} ban(s), {} voice mute(s), {} text mute(s).",
              CacheFor(PunishType::Ban).size(), CacheFor(PunishType::VoiceMute).size(),
              CacheFor(PunishType::TextMute).size());
    return true;
}

std::vector<Punishment> PunishmentManager::GetActive(PunishType kind) const
{
    const Cache& cache = CacheFor(kind);
    std::vector<Punishment> out;
    out.reserve(cache.size());
    for (const auto& [steamId, record] : cache)
    {
        if (!record.IsExpired())
            out.push_back(record);
    }
    std::sort(out.begin(), out.end(),
              [](const Punishment& a, const Punishment& b) { return a.CreatedAt > b.CreatedAt; });
    return out;
}

std::optional<Punishment> PunishmentManager::GetActive(PunishType kind, int64_t steamId)
{
    Cache& cache = CacheFor(kind);
    auto it = cache.find(steamId);
    if (it == cache.end())
        return std::nullopt;
    if (it->second.IsExpired())
    {
        cache.erase(it);
        if (kind == PunishType::VoiceMute)
            RefreshVoiceChannel(_rt, steamId, false);
        return std::nullopt;
    }
    return it->second;
}

bool PunishmentManager::IsPunished(PunishType kind, int64_t steamId) const
{
    return CacheFor(kind).contains(steamId);
}

bool PunishmentManager::Issue(Punishment& record)
{
    if (record.Kind == PunishType::Kick)
        return false;  // a kick stores no row; IssuePunishment applies and broadcasts it directly

    StampTimes(record);

    if (IsCached(record.Kind))
    {
        // Cache first so the punishment bites this frame; the id lands later, and only if
        // nothing has replaced the entry.
        Cache& cache = CacheFor(record.Kind);
        cache[record.TargetSteamId] = record;
        _repos.Punishments.CreateAsync(record, [&cache, steamId = record.TargetSteamId](int64_t id) {
            if (auto it = cache.find(steamId); it != cache.end() && it->second.Id == 0)
                it->second.Id = id;
        });
    }
    else
    {
        _repos.Punishments.CreateAsync(record);
    }

    if (record.Kind == PunishType::Ban)
    {
        if (auto* player = _rt.Players.BySteamId(record.TargetSteamId))
        {
            // The notice the connect-time reject builds, so both paths read the same.
            KickDeferred(player->Slot(), record.TargetSteamId,
                         BuildBanNotice(_rt.Translations, _config.GetAppeal(), record.Reason, record.ExpiresAt,
                                        record.TargetSteamId, player->Slot()));
        }
    }
    else if (record.Kind == PunishType::VoiceMute)
    {
        RefreshVoiceChannel(_rt, record.TargetSteamId, true);
    }

    _chat.BroadcastPunishment(std::string(IssuedKey(record.Kind)), record.AdminName, record.TargetName, record.Reason,
                              record.Duration);

    if (record.Kind == PunishType::Warn)
        EscalateWarning(record);
    return true;
}

void PunishmentManager::EscalateWarning(const Punishment& warning)
{
    // FIFO: this count sees the insert Issue just enqueued, and lands on the game thread.
    _repos.Punishments.CountActiveAsync(PunishType::Warn, warning.TargetSteamId, [this, w = warning](int active) {
        const int threshold = _config.GetPunishments().warningThreshold;
        if (threshold <= 0 || active < threshold)
            return;

        Log::Info("Warning threshold ({}) reached for {} -- escalating to ban.", threshold, w.TargetSteamId);
        _repos.Punishments.ClearAsync(PunishType::Warn, w.TargetSteamId);

        Punishment autoBan{.Kind = PunishType::Ban,
                           .TargetSteamId = w.TargetSteamId,
                           .TargetName = w.TargetName,
                           .AdminSteamId = w.AdminSteamId,
                           .AdminName = w.AdminName,
                           .Reason = _config.GetPunishments().defaultBanReason,
                           .Duration = 0};  // permanent escalation
        Issue(autoBan);
    });
}

bool PunishmentManager::Remove(PunishType kind, int64_t recordId, int64_t removedBy, const std::string& reason)
{
    Cache& cache = CacheFor(kind);
    auto it = std::ranges::find_if(cache, [recordId](const auto& entry) { return entry.second.Id == recordId; });
    if (it == cache.end())
        return false;  // already lifted, possibly by another server - nothing to remove

    const int64_t steamId = it->first;
    const std::string targetName = it->second.TargetName;
    cache.erase(it);

    _repos.Punishments.RemoveAsync(recordId, removedBy, reason);
    if (kind == PunishType::VoiceMute)
        RefreshVoiceChannel(_rt, steamId, false);
    _chat.BroadcastPunishment(std::string(LiftedKey(kind)), "Admin", targetName, reason, 0);
    return true;
}

bool PunishmentManager::RemoveBySteamId(PunishType kind, int64_t steamId, int64_t removedBy, const std::string& reason)
{
    // The caches mirror every active row, so a miss needs no database lookup.
    const Cache& cache = CacheFor(kind);
    auto it = cache.find(steamId);
    return it != cache.end() && Remove(kind, it->second.Id, removedBy, reason);
}

void PunishmentManager::KickDeferred(int slot, int64_t steamId, std::string reason)
{
    if (!VoltMod::IsValidSlot(slot))
        return;

    _pendingKick[slot] = _rt.Scheduler.NextTick([&rt = _rt, slot, steamId, reason = std::move(reason)] {
        // The seat can change hands before this fires; kicking whoever took it is not the ban.
        if (rt.Players.Get(VoltMod::PlayerRef{slot, steamId}))
            (void)rt.Entities.Controller(slot).Kick(reason);
    });
}

void PunishmentManager::ExpireOldPunishments()
{
    _repos.Punishments.ExpireOldAsync();
    // FIFO: this snapshot runs after the expirations above have landed.
    RefreshCachesAsync();
}

void PunishmentManager::RefreshCachesAsync()
{
    _repos.Punishments.FindAllActiveAsync([this](std::vector<Punishment> rows) { SwapCaches(std::move(rows)); });
}

void PunishmentManager::SwapCaches(std::vector<Punishment> rows)
{
    // Snapshot before the swap: voice mutes that expired this sweep still need a channel refresh.
    const Cache previouslyVoiceMuted = CacheFor(PunishType::VoiceMute);

    for (Cache& cache : _active)
        cache.clear();
    for (auto& row : rows)
    {
        if (IsCached(row.Kind))
            CacheFor(row.Kind)[row.TargetSteamId] = std::move(row);
    }

    const Cache& voiceMuted = CacheFor(PunishType::VoiceMute);
    for (const auto& [steamId, record] : previouslyVoiceMuted)
    {
        if (!voiceMuted.contains(steamId))
            RefreshVoiceChannel(_rt, steamId, false);
    }
}

}  // namespace AdminSystem::Punishments
