#pragma once

#include <CS2Kit/Players/Player.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace AdminSystem::Core
{

/**
 * Renders admin-system chat semantics on top of CS2Kit's chat helpers:
 * styled per-command replies, punishment broadcasts, and prefix-tagged admin chat.
 */
class ChatService
{
public:
    ChatService() = default;

    /**
     * Send a single-line reply to one player. Color codes inside `message` are honored;
     * `Default` color is prepended automatically when missing.
     */
    void Reply(int slot, std::string_view message);

    /** Reply with a translated "no permission" line. */
    void NoPermission(int slot);

    /**
     * Broadcast an issued punishment, e.g. "[ADMIN] Bob banned Alice for cheating (1d)".
     * Skipped when `chat.broadcastPunishments` is false.
     */
    void BroadcastPunishment(std::string_view action, std::string_view adminName, std::string_view targetName,
                             std::string_view reason, int64_t durationSec);

    /**
     * Broadcast a non-punishment admin action, e.g. "[ADMIN] Bob slapped Alice".
     * The verb is read from `translationKey`; both `adminName` and `targetName` are interpolated.
     * Pass an empty `targetName` for self-targeted or server-wide actions.
     */
    void BroadcastAction(const std::string& translationKey, std::string_view adminName, std::string_view targetName);

    /**
     * Re-emit an admin's regular chat with their group's colored prefix attached.
     * Caller is expected to SUPERCEDE the original say/say_team in the chat hook.
     */
    void RebroadcastAdminChat(const CS2Kit::Players::Player* admin, std::string_view message, bool teamOnly);

    /**
     * Apply admin-system semantics to a player's say/say_team message:
     * dispatch registered chat commands, drop messages from text-muted players, and rebroadcast
     * admin chat with a colored prefix. Returns true when the original message should be
     * superseded (the hook caller must skip the engine's default broadcast).
     */
    bool HandleSay(CS2Kit::Players::Player* player, std::string_view message, bool isSayTeam);

    /**
     * Notify a voice-muted player that the engine is suppressing their microphone. Rate-limited
     * to avoid spam: the SetClientListening hook fires once per (receiver, sender) pair every
     * time the player keys voice, which can easily hit dozens of calls in a single press.
     */
    void NotifyVoiceMuted(CS2Kit::Players::Player* player);

private:
    /** Last-notified timestamps per slot for the voice-mute notice (keyed by slot, value = epoch sec). */
    std::unordered_map<int, int64_t> _voiceMuteNoticeAt;
    /** Last-notified timestamps per slot for the text-mute notice. */
    std::unordered_map<int, int64_t> _textMuteNoticeAt;
};

}  // namespace AdminSystem::Core
