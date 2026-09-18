#pragma once

#include <VoltMod/Core/Slots/Slot.hpp>
#include <array>

namespace Anticheat
{

/** Per-slot deadlines for the adapters that poll clients, so the cadence lives in one place. A
 *  slot with no deadline gets its first one a delay out rather than firing the moment it joins. */
class SlotSchedule
{
public:
    /** True when @p slot's next run is due. A slot with none booked gets its first one
     *  @p firstDelaySec out and is not due until then. */
    bool IsDue(int slot, double nowSec, double firstDelaySec)
    {
        if (!VoltMod::IsValidSlot(slot))
            return false;

        double& next = _next[slot];
        if (next == NotScheduled)
        {
            next = nowSec + firstDelaySec;
            return false;
        }
        return nowSec >= next;
    }

    /** Book @p slot's next run @p delaySec from now. */
    void RunIn(int slot, double nowSec, double delaySec)
    {
        if (VoltMod::IsValidSlot(slot))
            _next[slot] = nowSec + delaySec;
    }

    /** Seconds until @p slot's next run, or 0 when it has none. Diagnostics only. */
    double TimeLeft(int slot, double nowSec) const
    {
        if (!VoltMod::IsValidSlot(slot) || _next[slot] == NotScheduled)
            return 0.0;
        return _next[slot] > nowSec ? _next[slot] - nowSec : 0.0;
    }

    void ClearSlot(int slot)
    {
        if (VoltMod::IsValidSlot(slot))
            _next[slot] = NotScheduled;
    }

    /** A map change clears every deadline; players who ride it out never connect again. */
    void ClearAll() { _next.fill(NotScheduled); }

private:
    static constexpr double NotScheduled = 0.0;

    std::array<double, VoltMod::MaxPlayers> _next{};
};

}  // namespace Anticheat
