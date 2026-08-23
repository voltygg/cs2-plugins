#pragma once

// Feeds InvalidCvarRules from two tiers: the userinfo copies the engine already holds, and the
// network convar query for everything else. Without _rt.ClientCvars the query tier is simply
// absent, so a degraded load falls back to userinfo rather than going blind.
//
// An unanswered query produces no callback at all, so nothing here waits on a reply or reads
// silence as evidence.

#include "Detectors/InvalidCvarRules.hpp"

#include <CS2Kit/Api.hpp>
#include <array>
#include <cstdint>
#include <random>
#include <string_view>

namespace Anticheat
{

class AntiCheatManager;

class InvalidCvarDetector
{
public:
    InvalidCvarDetector(AntiCheatManager& manager, CS2Kit::Runtime& runtime) : _manager(manager), _rt(runtime) {}

    /** Start the poll pump. Idempotent. */
    void Initialize();

    /** A player is in the server: arm their first poll. */
    void OnFullyConnected(int slot);

    void OnSlotChanged(int slot);
    void Reset();

    /** Seconds until @p slot's next poll, or 0 when it is not armed. Diagnostics only. */
    double PollsIn(int slot, double nowSec) const;

private:
    struct SlotState
    {
        double NextPoll = 0.0;  // 0 = not armed
        size_t Cursor = 0;      // where this slot's next batch starts in the queried tier
    };

    void Poll(int slot, SlotState& state);
    void ReadUserInfo(int slot);
    void OnReply(int slot, CS2Kit::ClientCvarStatus status, std::string_view name, std::string_view value);
    double NextDelaySec();

    AntiCheatManager& _manager;
    CS2Kit::Runtime& _rt;
    std::array<SlotState, MaxSlots> _slots{};
    std::minstd_rand _random;
    CS2Kit::Subscription _pollTimer;
};

}  // namespace Anticheat
