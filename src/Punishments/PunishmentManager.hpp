#pragma once

#include "../Database/Entities/Ban.hpp"
#include "../Database/Entities/TextMute.hpp"
#include "../Database/Entities/VoiceMute.hpp"
#include "../Database/Entities/Warning.hpp"

#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace AdminSystem::Punishments
{

using namespace CS2Kit::Core;

/**
 * Manages active punishments (bans, voice mutes, text mutes, warnings).
 * Caches active punishments in memory and syncs with the database.
 */
class PunishmentManager
{
public:
    PunishmentManager() = default;

    bool LoadActivePunishments();
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

    /** Mark expired bans/mutes inactive in the DB and rebuild the in-memory caches. */
    void ExpireOldPunishments();

private:
    std::unordered_map<int64_t, Database::Ban> _activeBans;             /**< keyed by TargetSteamId */
    std::unordered_map<int64_t, Database::VoiceMute> _activeVoiceMutes; /**< keyed by TargetSteamId */
    std::unordered_map<int64_t, Database::TextMute> _activeTextMutes;   /**< keyed by TargetSteamId */
    std::unordered_set<int64_t> _voiceMutedPlayers;                     /**< fast IsVoiceMuted() lookup */
    std::unordered_set<int64_t> _textMutedPlayers;                      /**< fast IsTextMuted() lookup */
};

}  // namespace AdminSystem::Punishments
