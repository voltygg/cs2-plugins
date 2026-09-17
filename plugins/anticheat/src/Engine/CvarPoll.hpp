#pragma once

#include "Engine/Detectors.hpp"
#include "Detect/Rules/InvalidCvar.hpp"

#include <VoltMod/Api.hpp>
#include <array>
#include <cstdint>
#include <random>
#include <optional>
#include <string_view>

namespace Anticheat
{

class CvarPoll
{
public:
    CvarPoll(Detectors& detectors, VoltMod::Runtime& runtime) : _detectors(detectors), _rt(runtime) {}

    /** Start the repeating poll timer. Idempotent. */
    void Initialize();

    void OnSlotChanged(int slot);
    void Reset();

    /** Seconds until @p slot's next poll, or 0 when none is scheduled. Diagnostics only. */
    double PollsIn(int slot, double nowSec) const;

private:
    struct SlotState
    {
        double NextPoll = 0.0;  // 0 = not scheduled
        size_t Cursor = 0;      // where this slot's next batch starts in the queried tier
    };

    void Poll(int slot, SlotState& state);
    void ReadUserInfo(int slot);
    void OnReply(int slot, VoltMod::ClientConVarStatus status, std::string_view name, std::string_view value);
    /** Turn a newly invalid reading into evidence. A confirmed bad value is a whole unit of it. */
    void ReportVerdict(int slot, const std::optional<Rules::CvarVerdict>& verdict);
    double NextDelaySec();

    Detectors& _detectors;
    VoltMod::Runtime& _rt;
    std::array<SlotState, MaxSlots> _slots{};
    std::minstd_rand _random;
    VoltMod::Subscription _pollTimer;
};

}  // namespace Anticheat
