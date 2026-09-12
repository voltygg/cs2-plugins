#pragma once

#include "../Config/ConfigManager.hpp"
#include "../Core/ChatService.hpp"
#include "../Database/Entities/Ban.hpp"
#include "../Database/Entities/TextMute.hpp"
#include "../Database/Entities/VoiceMute.hpp"
#include "../Database/Entities/Warning.hpp"

#include <VoltMod/Database/Api.hpp>
#include <VoltMod/Runtime.hpp>
#include <algorithm>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace AdminSystem::Punishments
{

/**
 * Manages active bans, mutes, and warnings. Gameplay reads in-memory caches, which are updated
 * synchronously; database writes run asynchronously, failures are logged, and inserted row IDs
 * are backfilled when they return.
 */
class PunishmentManager
{
public:
    PunishmentManager(VoltMod::Database& db, const Config::ConfigManager& config, VoltMod::Runtime& runtime,
                      Core::ChatService& chat)
        : _db(db), _config(config), _rt(runtime), _chat(chat)
    {}

    bool LoadActivePunishments();
    /** Snapshot of the cached active bans, newest first (drives the unban menu). */
    std::vector<Database::Ban> GetActiveBans() const;
    /** Snapshots of cached active voice and text mutes, newest first. */
    std::vector<Database::VoiceMute> GetActiveVoiceMutes() const;
    std::vector<Database::TextMute> GetActiveTextMutes() const;
    std::optional<Database::Ban> GetActiveBan(int64_t steamId);
    std::optional<Database::VoiceMute> GetActiveVoiceMute(int64_t steamId);
    std::optional<Database::TextMute> GetActiveTextMute(int64_t steamId);
    bool IsVoiceMuted(int64_t steamId);
    bool IsTextMuted(int64_t steamId);

    /** Issue a ban: persist, kick the player if online, broadcast. */
    bool IssueBan(Database::Ban& ban);
    /** Persist and broadcast a voice mute, then suppress the target through SetClientListening. */
    bool IssueVoiceMute(Database::VoiceMute& mute);
    /** Issue a text mute; Plugin.cpp drops messages from text-muted players. */
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
     * Defer the kick because callers run inside target hooks and disconnecting there would interrupt
     * the virtual call. The per-slot timer is replaced on reconnect and released on unload.
     */
    void KickDeferred(int slot, int64_t steamId, std::string reason);

private:
    VoltMod::Database& _db;
    const Config::ConfigManager& _config;
    VoltMod::Runtime& _rt;
    Core::ChatService& _chat;
    VoltMod::PerSlot<VoltMod::Subscription> _pendingKick;

    /** Re-query the three active lists off-thread and swap the caches when the rows arrive. */
    void RefreshCachesAsync();

    // Shared implementation for active ban, voice-mute, and text-mute snapshots.
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

    // The caches mirror all active rows, so a miss needs no database lookup.
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
