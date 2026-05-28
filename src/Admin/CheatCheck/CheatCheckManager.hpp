#pragma once

#include "CheatCheckMode.hpp"

#include "../../Web/HttpResult.hpp"

#include <CS2Kit/Core/Singleton.hpp>
#include <array>
#include <cstdint>
#include <string>

namespace AdminSystem::Admin::CheatCheck
{

constexpr int MaxSlots = 64;

/** Per-target pending cheat-check state. Indexed by the suspect's slot. */
struct PendingCheck
{
    bool Active = false;
    int AdminSlot = -1;
    int64_t AdminSteamId = 0;
    CheatCheckMode Mode = CheatCheckMode::FixedLink;
    int64_t DeadlineSec = 0;  // Unix timestamp (TimeUtils::Now) at which the check times out
    uint64_t TickTimer = 0;
    std::string ResolvedUrl;  // URL shown to the suspect (empty while awaiting / before playerProvided submit)
    bool AwaitingUrl = false;
    uint64_t RequestSeq = 0;  // staleness guard for async HTTP completions
};

/**
 * Owns all pending cheat checks: freezes the suspect, shows a persistent center-HTML panel +
 * chat link, runs a countdown, and auto-kicks (or unfreezes) on timeout. All methods run on the
 * game thread; async HTTP completions are marshalled back via HttpClient::Drain before reaching here.
 */
class CheatCheckManager : public CS2Kit::Core::Singleton<CheatCheckManager>
{
public:
    explicit CheatCheckManager(Token) {}

    enum class SubmitResult
    {
        NoActiveCheck,
        Invalid,
        Relayed,
    };

    /** Start a check on `targetSlot` called by `adminSlot`. Re-call replaces any existing check. */
    bool StartCheck(int adminSlot, int targetSlot);

    /** Admin-initiated cancel: unfreeze, clear the panel, broadcast cleared. */
    void Cancel(int targetSlot);

    /** A suspect submitted a link via `!cc` (playerProvided mode). */
    SubmitResult SubmitPlayerLink(int callerSlot, const std::string& link);

    bool IsPending(int targetSlot) const;

    /** Silent teardown for a disconnecting slot (no unfreeze/broadcast — the player is gone). */
    void CancelAllForSlot(int slot);

    /** Tear down every active check (plugin unload). */
    void CancelAll();

private:
    void Tick(int targetSlot);
    void Expire(int targetSlot);
    void ResetCheck(int targetSlot);  // cancel timer + clear panel + reset state, silently
    void SendPanel(int targetSlot);
    void SendChatInstructions(int targetSlot);
    void Unfreeze(int targetSlot);
    void ResolveUrl(int targetSlot);
    void RequestRoom(int targetSlot);
    void OnRoomResponse(int targetSlot, uint64_t seq, const Web::HttpResult& result);
    void OnRoomFailed(int targetSlot);
    void RelayCheckerUrl(int targetSlot, const std::string& checkerUrl);

    bool ValidSlot(int slot) const { return slot >= 0 && slot < MaxSlots; }

    std::array<PendingCheck, MaxSlots> _checks{};
    uint64_t _seq = 1;
};

}  // namespace AdminSystem::Admin::CheatCheck
