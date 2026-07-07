#pragma once

#include <CS2Kit/Api.hpp>
#include <array>
#include <chrono>
#include <cstdint>

namespace Anticheat
{

/** Monotonic seconds for score decay and event windows (wall clock, not game time). */
inline double NowSeconds()
{
    using Clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}

/** Fixed ring of event timestamps; answers "how many events inside the window ending now". */
class EventRing
{
public:
    void Push(double now)
    {
        _times[_head] = now;
        _head = (_head + 1) % Capacity;
        if (_count < Capacity)
            ++_count;
    }

    int CountWithin(double now, double windowSec) const
    {
        int inWindow = 0;
        for (int i = 0; i < _count; ++i)
            if (now - _times[i] <= windowSec)
                ++inWindow;
        return inWindow;
    }

private:
    static constexpr int Capacity = 16;
    std::array<double, Capacity> _times{};
    int _head = 0;
    int _count = 0;
};

/**
 * Everything the detectors track for one slot. Value-reset on connect/disconnect
 * via PerSlot<PlayerState>::BindReset(), so nothing leaks across occupants.
 */
struct PlayerState
{
    // Input feed bookkeeping (ticks are our own usercmd counter, not engine ticks).
    uint32_t Tick = 0;
    bool HasPrevAngles = false;
    float PrevYaw = 0.0f;
    float PrevPitch = 0.0f;

    // Spin tracking: rolling window of per-tick yaw deltas.
    static constexpr int SpinWindow = 32;
    std::array<float, SpinWindow> YawDeltas{};
    int YawDeltaHead = 0;
    int YawDeltaCount = 0;
    float YawDeltaSum = 0.0f;   // sum of |delta| across the window
    uint32_t SpinTicks = 0;     // consecutive ticks above the spin velocity threshold
    uint32_t LastSpinTick = 0;  // most recent tick that qualified as spinning
    double LastSpinOnlyReport = 0.0;
    EventRing SpinKills;

    // Fire-tick snapshot for on-target correlation.
    uint32_t LastFireTick = 0;
    float FireYaw = 0.0f;
    float FirePitch = 0.0f;
    bool HasFired = false;

    // Aim-snap: last detected pre-fire flick awaiting damage confirmation.
    bool SnapPending = false;
    uint32_t SnapFireTick = 0;
    EventRing SnapHits;

    // Silent aim: last angle jump without matching mouse input.
    bool SilentPending = false;
    uint32_t SilentTick = 0;
    EventRing SilentHits;

    // Angle sanity.
    int BadAngleTicks = 0;
    EventRing SanityEvents;

    // Flash blindness (tracked from the player_blind event).
    double BlindUntil = 0.0;
    EventRing BlindKills;
};

}  // namespace Anticheat
