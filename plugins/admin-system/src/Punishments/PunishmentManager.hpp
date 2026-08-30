#pragma once

#include "../Config/ConfigManager.hpp"
#include "../Core/ChatService.hpp"
#include "../Database/Entities/Ban.hpp"
#include "../Database/Entities/TextMute.hpp"
#include "../Database/Entities/VoiceMute.hpp"
#include "../Database/Entities/Warning.hpp"

#include <VoltMod/Database/PostgresDatabase.hpp>
#include <VoltMod/Runtime.hpp>
#include <algorithm>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace AdminSystem::Punishments
{

/**
 * Manages active punishments (bans, voice mutes, text mutes, warnings).
 *
 * Cache-first: every gameplay decision (ban-on-connect, IsVoiceMuted, the unban/unmute menus)
 * reads the in-memory caches, which are updated synchronously when a punishment is issued or
 * removed. The database writes ride the async worker - a failed write is logged, never blocks
 * the game thread, and the row id from an insert is backfilled into the cache when it lands.
 */
class PunishmentManager
{
public:
    PunishmentManager(VoltMod::PostgresDatabase& db, const Config::ConfigManager& config, VoltMod::Runtime& runtime,
                      Core::ChatService& chat)
        : _db(db), _config(config), _rt(runtime), _chat(chat)
    {}

    bool LoadActivePunishments();
    /** Snapshot of the cached active bans, newest first (drives the unban menu). */
    std::vector<Database::Ban> GetActiveBans() const;
    /** Snapshots of the cached active voice/text mutes, newest first (drive the unmute menu). */
    std::vector<Database::VoiceMute> GetActiveVoiceMutes() const;
    std::vector<Database::TextMute> GetActiveTextMutes() const;
    std::optional<Database::Ban> GetActiveBan(int64_t steamId);
    std::optional<Database::VoiceMute> GetActiveVoiceMute(int64_t steamId);
    std::optional<Database::TextMute> GetActiveTextMute(int64_t steamId);
    bool IsVoiceMuted(int64_t steamId);
    bool IsTextMuted(int64_t steamId);

    /** Issue a ban: persist, kick the player if online, broadcast. */
    bool IssueBan(Database::Ban& ban);
    /**
     * Issue a voice mute: persist, broadcast, and immediately suppress the target's outbound
     * voice for every connected listener via the SetClientListening hook.
     */
    bool IssueVoiceMute(Database::VoiceMute& mute);
    /** Issue a text mute: persist, broadcast. The chat hook in Plugin.cpp drops messages from text-muted players. */
    bool IssueTextMute(Database::TextMute& mute);
    /** Issue a warning: persist, broadcast, and auto-ban once the configured threshold is reached. */
    bool IssueWarning(Database::Warning& warning);

    /** Remove (un-ban / un-voice-mute / un-text-mute) the active punishment for `steamId`. Returns false if none. */
    bool RemoveBanBySteamId(int64_t steamId, int64_t removedBy, const std::string& reason);
    bool RemoveVoiceMuteBySteamId(int64_t steamId, int64_t removedBy, const std::string& reason);
    bool RemoveTextMuteBySteamId(int64_t steamId, int64_t removedBy, const std::string& reason);

    /** Remove the active punishment by row id. Returns false if it was already inactive / unknown. */
    bool RemoveBan(int64_t banId, int64_t removedBy, const std::string& reason);
    bool RemoveVoiceMute(int64_t muteId, int64_t removedBy, const std::string& reason);
    bool RemoveTextMute(int64_t muteId, int64_t removedBy, const std::string& reason);

    /** Mark expired bans/mutes inactive in the DB and rebuild the in-memory caches (all async). */
    void ExpireOldPunishments();

    /**
     * Kick @p slot on the next game frame, but only if @p steamId still occupies it.
     *
     * Deferred because both callers reach this from inside an engine hook on the target - the
     * connect hook and, through IAdminActions, an anticheat detection - where kicking disconnects
     * the client mid-virtual-call. The timer is owned per slot, so a reconnect replaces it and
     * unload drops it.
     */
    void KickDeferred(int slot, int64_t steamId, std::string reason);

private:
    VoltMod::PostgresDatabase& _db;
    const Config::ConfigManager& _config;
    VoltMod::Runtime& _rt;
    Core::ChatService& _chat;
    VoltMod::PerSlot<VoltMod::Subscription> _pendingKick;

    /** Re-query the three active lists off-thread and swap the caches when the rows arrive. */
    void RefreshCachesAsync();

    // Shared body for the three GetActive* snapshots: copy the non-expired cache entries out,
    // newest first. Keyed the same way for bans/voice/text mutes.
    template <typename TEntity>
    static std::vector<TEntity> SnapshotActive(const std::unordered_map<int64_t, TEntity>& cache)
    {
        std::vector<TEntity> out;
        out.reserve(cache.size());
        for (const auto& [steamId, entity] : cache)
        {
            if (!entity.IsExpired())
                out.push_back(entity);
        }
        std::sort(out.begin(), out.end(), [](const TEntity& a, const TEntity& b) { return a.CreatedAt > b.CreatedAt; });
        return out;
    }

    // Shared body for the three Remove*BySteamId methods. The caches mirror ALL active rows
    // (shared across servers, refreshed by the sweep), so a cache miss means no active row -
    // no DB fallback lookup needed.
    using RemoveByIdFn = bool (PunishmentManager::*)(int64_t, int64_t, const std::string&);

    template <typename TEntity>
    bool RemoveBySteamIdImpl(std::unordered_map<int64_t, TEntity>& cache, int64_t steamId, int64_t removedBy,
                             const std::string& reason, RemoveByIdFn remove)
    {
        auto it = cache.find(steamId);
        if (it != cache.end())
            return (this->*remove)(it->second.Id, removedBy, reason);
        return false;
    }

    std::unordered_map<int64_t, Database::Ban> _activeBans;             /**< keyed by TargetSteamId */
    std::unordered_map<int64_t, Database::VoiceMute> _activeVoiceMutes; /**< keyed by TargetSteamId */
    std::unordered_map<int64_t, Database::TextMute> _activeTextMutes;   /**< keyed by TargetSteamId */
    std::unordered_set<int64_t> _voiceMutedPlayers;                     /**< fast IsVoiceMuted() lookup */
    std::unordered_set<int64_t> _textMutedPlayers;                      /**< fast IsTextMuted() lookup */
};

}  // namespace AdminSystem::Punishments
