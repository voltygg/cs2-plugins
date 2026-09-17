#pragma once

#include "Detect/Rules/InvalidCvar.hpp"
#include "Engine/Detectors.hpp"
#include "Engine/SlotSchedule.hpp"

#include <VoltMod/Api.hpp>
#include <array>
#include <cstdint>
#include <optional>
#include <random>
#include <string_view>

namespace Anticheat
{

class CvarPoll
{
public:
    CvarPoll(Detectors& detectors, VoltMod::Runtime& runtime) : _detectors(detectors), _rt(runtime) {}

    /** Start the repeating poll timer. Idempotent. */
    void Initialize();

    void ClearSlot(int slot);
    void Reset();

    /** Seconds until @p slot's next poll, or 0 when none is scheduled. Diagnostics only. */
    double PollsIn(int slot, double nowSec) const;

private:
    void Poll(int slot);
    void ReadUserInfo(int slot);
    void OnReply(int slot, VoltMod::ClientConVarStatus status, std::string_view name, std::string_view value);
    /** Turn a newly invalid reading into evidence. A confirmed bad value is a whole unit of it. */
    void ReportVerdict(int slot, const std::optional<Rules::CvarVerdict>& verdict);
    double NextDelaySec();

    Detectors& _detectors;
    VoltMod::Runtime& _rt;
    SlotSchedule _schedule;
    /** Where each slot's next batch starts in the queried tier. */
    std::array<size_t, MaxSlots> _cursor{};
    std::minstd_rand _random;
    VoltMod::Subscription _pollTimer;
};

}  // namespace Anticheat
