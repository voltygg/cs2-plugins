#pragma once

#include "../Database/Entities/Ban.hpp"
#include "../Database/Entities/Gag.hpp"
#include "../Database/Entities/Mute.hpp"
#include "../Database/Entities/Warning.hpp"

#include <CS2Kit/Core/Singleton.hpp>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace AdminSystem::Punishments
{

using namespace CS2Kit::Core;

/**
 * Manages active punishments (bans, mutes, gags, warnings).
 * Caches active punishments in memory and syncs with the database.
 */
class PunishmentManager : public Singleton<PunishmentManager>
{
public:
    explicit PunishmentManager(Token) {}

    bool LoadActivePunishments();
    std::optional<Database::Ban> GetActiveBan(int64_t steamId);
    std::optional<Database::Mute> GetActiveMute(int64_t steamId);
    std::optional<Database::Gag> GetActiveGag(int64_t steamId);
    bool IsMuted(int64_t steamId);
    bool IsGagged(int64_t steamId);

    /** Issue a ban: persist, kick the player if online, broadcast. */
    bool IssueBan(Database::Ban& ban);
    /** Issue a mute: persist, broadcast. Voice suppression is not implemented yet; chat is unaffected. */
    bool IssueMute(Database::Mute& mute);
    /** Issue a gag: persist, broadcast. The chat hook in Plugin.cpp drops messages from gagged players. */
    bool IssueGag(Database::Gag& gag);
    /** Issue a warning: persist, broadcast, and auto-ban once the configured threshold is reached. */
    bool IssueWarning(Database::Warning& warning);

    /** Remove (un-ban/un-mute/un-gag) the active punishment for `steamId`. Returns false if none. */
    bool RemoveBanBySteamId(int64_t steamId, int64_t removedBy, const std::string& reason);
    bool RemoveMuteBySteamId(int64_t steamId, int64_t removedBy, const std::string& reason);
    bool RemoveGagBySteamId(int64_t steamId, int64_t removedBy, const std::string& reason);

    /** Remove the active punishment by row id. Returns false if it was already inactive / unknown. */
    bool RemoveBan(int64_t banId, int64_t removedBy, const std::string& reason);
    bool RemoveMute(int64_t muteId, int64_t removedBy, const std::string& reason);
    bool RemoveGag(int64_t gagId, int64_t removedBy, const std::string& reason);

    /** Mark expired bans/mutes/gags inactive in the DB and rebuild the in-memory caches. */
    void ExpireOldPunishments();

private:
    std::unordered_map<int64_t, Database::Ban> _activeBans;    /**< keyed by TargetSteamId */
    std::unordered_map<int64_t, Database::Mute> _activeMutes;  /**< keyed by TargetSteamId */
    std::unordered_map<int64_t, Database::Gag> _activeGags;    /**< keyed by TargetSteamId */
    std::unordered_set<int64_t> _mutedPlayers;                 /**< fast IsMuted() lookup */
    std::unordered_set<int64_t> _gaggedPlayers;                /**< fast IsGagged() lookup */
};

}  // namespace AdminSystem::Punishments
