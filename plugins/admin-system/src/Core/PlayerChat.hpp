#pragma once

#include "../Admin/AdminManager.hpp"
#include "../Config/ConfigManager.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "ChatService.hpp"

#include <VoltMod/Core/Throttle.hpp>
#include <VoltMod/Players/Player.hpp>
#include <VoltMod/Runtime.hpp>
#include <cstdint>
#include <optional>
#include <string_view>

namespace AdminSystem::Core
{

/**
 * The inbound half of chat: what a player says, and what they are allowed to say.
 *
 * Split from @ref ChatService because this side reads admin and punishment state while the
 * output side is read by those same managers - keeping both in one class made the object graph
 * cyclic. Constructed after them, so it holds them directly.
 */
class PlayerChat
{
public:
    PlayerChat(VoltMod::Runtime& runtime, const Config::ConfigManager& config, ChatService& chat,
               Admin::AdminManager& admins, Punishments::PunishmentManager& punishments)
        : _rt(runtime), _config(config), _chat(chat), _admins(admins), _punishments(punishments)
    {}

    /**
     * Apply admin-system semantics to a player's say/say_team message:
     * dispatch registered chat commands, drop messages from text-muted players, and rebroadcast
     * admin chat with a colored prefix. Returns true when the original message should be
     * superseded (the hook caller must skip the engine's default broadcast).
     */
    bool HandleSay(VoltMod::Player* player, std::string_view message, bool isSayTeam);

    /**
     * Re-emit an admin's regular chat with their group's colored prefix attached.
     * Caller is expected to SUPERCEDE the original say/say_team in the chat hook.
     */
    void RebroadcastAdminChat(const VoltMod::Player* admin, std::string_view message, bool teamOnly);

    /**
     * Notify a voice-muted player that the engine is suppressing their microphone. Rate-limited
     * to avoid spam: the SetClientListening hook fires once per (receiver, sender) pair every
     * time the player keys voice, which can easily hit dozens of calls in a single press.
     */
    void NotifyVoiceMuted(VoltMod::Player* player);

private:
    VoltMod::Runtime& _rt;
    const Config::ConfigManager& _config;
    ChatService& _chat;
    Admin::AdminManager& _admins;
    Punishments::PunishmentManager& _punishments;

    // Once per minute per player: the voice hook fires every keypress and chat spam produces
    // dozens of say events, so unthrottled notices would out-spam the spam itself.
    static constexpr int64_t MuteNoticeIntervalSec = 60;

    /** The red "you are muted" line plus its optional reason, for either mute kind. The two
     *  entities share ExpiresAt/Reason, so the notice differs only in which key names it. */
    template <class TMute>
    void ReplyMuteNotice(int slot, std::string_view noticeKey, const std::optional<TMute>& mute);

    VoltMod::Throttle<int> _voiceMuteNotice{MuteNoticeIntervalSec};
    VoltMod::Throttle<int> _textMuteNotice{MuteNoticeIntervalSec};
};

}  // namespace AdminSystem::Core
