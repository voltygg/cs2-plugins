#pragma once

#include "../../Web/HttpResult.hpp"
#include "PendingCheck.hpp"

#include <CS2Kit/Sdk/Entity.hpp>
#include <CS2Kit/Sdk/MoveType.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <string>

namespace AdminSystem::Admin::CheatCheck
{

constexpr int MaxSlots = CS2Kit::Sdk::MaxPlayers;

/**
 * Owns all pending cheat checks: freezes the suspect, shows a persistent center-HTML panel +
 * chat link, runs a countdown, and auto-kicks (or unfreezes) on timeout. All methods run on the
 * game thread; async HTTP completions are marshalled back via HttpClient::DispatchCompletions before reaching here.
 */
class CheatCheckManager
{
public:
    CheatCheckManager() = default;

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

    /** A suspect submitted a link via `!cc` (playerProvided mode). */
    SubmitResult SubmitPlayerLink(int callerSlot, const std::string& link);

    /** Silent teardown for a disconnecting slot (no unfreeze/broadcast — the player is gone). */
    void CancelAllForSlot(int slot);

    /** Tear down every active check (plugin unload). */
    void CancelAll();

private:
    void Tick(int targetSlot);
    void Expire(int targetSlot);
    void ResetCheck(int targetSlot);  // cancel timer + clear panel + reset state, silently
    void Unfreeze(int targetSlot, CS2Kit::Sdk::MoveType restore);
    void ResolveUrl(int targetSlot);
    void RequestRoom(int targetSlot);
    void OnRoomResponse(int targetSlot, uint64_t seq, const Web::HttpResult& result);
    void OnRoomFailed(int targetSlot);
    void RelayCheckerUrl(int targetSlot, const std::string& checkerUrl);

    void FallbackToFixed(PendingCheck& pc);  // drop awaiting state, use the configured fixed link if any

    // Reply to the admin who called the check, guarding against a disconnected/replaced admin slot.
    // `buildMessage` runs inside the admin's translation scope so per-admin language is honored.
    void ReplyToAdmin(const PendingCheck& pc, const std::function<std::string()>& buildMessage);

    bool ValidSlot(int slot) const { return slot >= 0 && slot < MaxSlots; }

    std::array<PendingCheck, MaxSlots> _checks{};
    uint64_t _seq = 1;
};

}  // namespace AdminSystem::Admin::CheatCheck
