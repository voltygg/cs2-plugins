#pragma once

#include "Singleton.hpp"

#include <cstdint>
#include <functional>
#include <vector>

namespace AdminSystem::Core
{

/**
 * Tick-based task scheduler. Supports one-shot delays and repeating timers.
 * Driven by OnGameFrame() called from Plugin::Hook_GameFrame().
 */
class Scheduler : public Singleton<Scheduler>
{
public:
    explicit Scheduler(Token) {}

    /** Schedule a one-shot callback after delayMs milliseconds. */
    uint64_t Delay(int64_t delayMs, std::function<void()> callback);

    /** Schedule a repeating callback every intervalMs milliseconds. */
    uint64_t Repeat(int64_t intervalMs, std::function<void()> callback);

    /** Schedule a one-shot delay, then repeat at intervalMs. */
    uint64_t DelayAndRepeat(int64_t delayMs, int64_t intervalMs, std::function<void()> callback);

    /** Execute callback on the next frame. */
    uint64_t NextTick(std::function<void()> callback);

    /** Cancel a scheduled timer by ID. */
    void Cancel(uint64_t id);

    /** Cancel all timers. */
    void CancelAll();

    /** Called from Plugin::Hook_GameFrame(). Processes expired timers. */
    void OnGameFrame();

private:
    struct Timer
    {
        uint64_t Id;
        int64_t NextFireTime;
        int64_t Interval;  // 0 = one-shot, >0 = repeating
        std::function<void()> Callback;
    };

    int64_t GetCurrentTimeMs() const;

    std::vector<Timer> _timers;
    uint64_t _nextId = 1;
};

}  // namespace AdminSystem::Core
