#pragma once

#include "Singleton.hpp"

#include <cstdint>
#include <functional>
#include <vector>

namespace AdminSystem::Core
{

/**
 * Tick-based task scheduler for one-shot delays and repeating timers.
 *
 * Driven by OnGameFrame() which is called every server tick from
 * Plugin::Hook_GameFrame(). Uses `std::chrono::steady_clock` for timing.
 *
 * All callbacks execute on the game thread (no synchronization needed).
 *
 * @par Usage
 * @code
 * // Fire once after 3 seconds:
 * auto id = Scheduler::Instance().Delay(3000, [] { Log::Info("3s elapsed"); });
 *
 * // Repeat every 5 seconds:
 * auto id = Scheduler::Instance().Repeat(5000, [] { Log::Info("tick"); });
 *
 * // Cancel:
 * Scheduler::Instance().Cancel(id);
 * @endcode
 */
class Scheduler : public Singleton<Scheduler>
{
public:
    explicit Scheduler(Token) {}

    /**
     * @brief Schedule a one-shot callback after a delay.
     * @param delayMs Delay in milliseconds before the callback fires.
     * @param callback Function to invoke when the delay expires.
     * @return Timer ID that can be passed to Cancel().
     */
    uint64_t Delay(int64_t delayMs, std::function<void()> callback);

    /**
     * @brief Schedule a repeating callback at a fixed interval.
     * @param intervalMs Interval in milliseconds between invocations.
     * @param callback Function to invoke on each interval tick.
     * @return Timer ID that can be passed to Cancel().
     */
    uint64_t Repeat(int64_t intervalMs, std::function<void()> callback);

    /**
     * @brief Schedule a callback that fires after an initial delay, then repeats.
     * @param delayMs   Initial delay in milliseconds before the first invocation.
     * @param intervalMs Interval in milliseconds between subsequent invocations.
     * @param callback  Function to invoke.
     * @return Timer ID that can be passed to Cancel().
     */
    uint64_t DelayAndRepeat(int64_t delayMs, int64_t intervalMs, std::function<void()> callback);

    /**
     * @brief Execute a callback on the next game frame.
     *
     * Equivalent to `Delay(0, callback)`. Useful for deferring work to the
     * next tick to avoid reentrancy issues.
     *
     * @param callback Function to invoke on the next frame.
     * @return Timer ID that can be passed to Cancel().
     */
    uint64_t NextTick(std::function<void()> callback);

    /**
     * @brief Cancel a scheduled timer by its ID.
     * @param id Timer ID returned by Delay(), Repeat(), DelayAndRepeat(), or NextTick().
     */
    void Cancel(uint64_t id);

    /**
     * @brief Cancel all active timers.
     *
     * Called during plugin shutdown to ensure no callbacks fire after subsystems
     * have been torn down.
     */
    void CancelAll();

    /**
     * Process expired timers. Called once per tick from Hook_GameFrame().
     *
     * Fires callbacks for timers whose NextFireTime has elapsed, advances
     * repeating timers, and removes completed one-shot timers.
     *
     * @note Safe against timer mutations during callbacks: expired indices are
     *       collected before invocation; removals happen in a separate pass.
     */
    void OnGameFrame();

private:
    /** Internal timer entry. */
    struct Timer
    {
        uint64_t Id;                     ///< Unique timer identifier.
        int64_t NextFireTime;            ///< Absolute time (ms) when the timer should fire next.
        int64_t Interval;                ///< 0 = one-shot, >0 = repeating interval in ms.
        std::function<void()> Callback;  ///< The function to invoke.
    };

    /**
     * Get the current time in milliseconds from a steady clock.
     * @return Milliseconds since an unspecified epoch (monotonically increasing).
     */
    int64_t GetCurrentTimeMs() const;

    std::vector<Timer> _timers;  ///< Active timer list.
    uint64_t _nextId = 1;        ///< Monotonically increasing ID counter.
};

}  // namespace AdminSystem::Core
