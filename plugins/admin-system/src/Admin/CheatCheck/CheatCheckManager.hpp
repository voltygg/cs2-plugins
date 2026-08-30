#pragma once

#include "../../Config/ConfigManager.hpp"
#include "../../Core/ChatService.hpp"
#include "CheatCheckView.hpp"
#include "PendingCheck.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Entities/Entity.hpp>
#include <VoltMod/Entities/MoveType.hpp>
#include <VoltMod/Http/HttpResult.hpp>
#include <VoltMod/Messaging/CenterHtml.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace AdminSystem::Admin::CheatCheck
{

constexpr int MaxSlots = VoltMod::MaxPlayers;

/**
 * Owns all pending cheat checks: freezes the suspect, shows a persistent center-HTML panel +
 * chat link, runs a countdown, and auto-kicks (or unfreezes) on timeout. All methods run on the
 * game thread; async HTTP completions are marshalled back via HttpClient::DispatchCompletions before reaching here.
 */
class CheatCheckManager
{
public:
    CheatCheckManager(VoltMod::Runtime& runtime, const Config::ConfigManager& config, Core::ChatService& chat)
        : _rt(runtime), _config(config), _chat(chat)
    {}

    enum class SubmitResult
    {
        NoActiveCheck,
        Invalid,
        Relayed,
    };

    /** Start a check on `targetSlot` called by `adminSlot`. Re-call replaces any existing check. */
    bool StartCheck(int adminSlot, int targetSlot);

    /** Admin-initiated cancel: unfreeze, clear the panel, broadcast cleared. Returns false if no check was active. */
    bool Cancel(int targetSlot);

    /** True while a check is pending on `slot` (drives menu enablement). */
    bool IsActive(int slot) const { return ValidSlot(slot) && _checks[slot].Active; }

    /** A suspect submitted a link via `!cc` (playerProvided mode). */
    SubmitResult SubmitPlayerLink(int callerSlot, const std::string& link);

    /** Silent teardown for a disconnecting slot (no unfreeze/broadcast - the player is gone). */
    void CancelAllForSlot(int slot);

    /** Tear down every active check (plugin unload). */
    void CancelAll();

private:
    VoltMod::Runtime& _rt;
    const Config::ConfigManager& _config;
    Core::ChatService& _chat;
    CheatCheckView _view{_rt, _config, _chat};
    /** Owns the panel re-send loop; CS2 drops center-HTML within a second or two. Declared after
     *  _view because its render callback reads through it. */
    VoltMod::CenterHtml _panel{_rt.Messages, _rt.Scheduler};

    void Tick(int targetSlot);
    void Expire(int targetSlot);
    void ShowPanel(int targetSlot);   // start (or restart) the panel's own refresh loop
    void ResetCheck(int targetSlot);  // cancel timer + clear panel + reset state, silently
    void Unfreeze(int targetSlot, VoltMod::MoveType restoreMove, int restoreTeam);
    void ResolveUrl(int targetSlot);
    void RequestRoom(int targetSlot);
    void OnRoomResponse(int targetSlot, uint64_t seq, const VoltMod::HttpResult& result);
    void OnRoomFailed(int targetSlot);
    void RelayCheckerUrl(int targetSlot, const std::string& checkerUrl);

    // Presence polling (CheatCheckPolling.cpp): pauses the countdown while the suspect
    // is in the check room and resumes it if they leave.
    void PollPresenceIfDue(int targetSlot);
    void OnPresenceResponse(int targetSlot, uint64_t seq, const VoltMod::HttpResult& result);

    void FallbackToFixed(PendingCheck& pc);  // drop awaiting state, use the configured fixed link if any

    // Slot of the admin who called the check, or nullopt when they disconnected / the slot
    // now hosts a different player.
    std::optional<int> ResolveAdminSlot(const PendingCheck& pc) const;

    // Reply to the admin who called the check, guarding against a disconnected/replaced admin slot.
    void ReplyToAdmin(const PendingCheck& pc, const std::function<std::string()>& buildMessage);

    bool ValidSlot(int slot) const { return slot >= 0 && slot < MaxSlots; }

    std::array<PendingCheck, MaxSlots> _checks{};
    uint64_t _seq = 1;
};

}  // namespace AdminSystem::Admin::CheatCheck
