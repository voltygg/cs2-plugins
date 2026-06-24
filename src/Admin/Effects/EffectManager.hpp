#pragma once

#include "EffectId.hpp"

#include <array>
#include <cstdint>
#include <functional>

namespace AdminSystem::Admin::Effects
{

constexpr int MaxSlots = 64;

/**
 * @brief Per-target effect bookkeeping. Owns scheduler handles and a CancelFn that
 * each effect populates to undo its own state (restore render mode, kill timers, etc.).
 */
struct ActiveEffect
{
    bool Active = false;
    bool RoundScoped = false;
    uint64_t TimerHandle = 0;
    uint64_t DurationHandle = 0; /**< Auto-expire timer, owned here and cancelled with the effect. */
    std::function<void()> CancelFn;
};

/** What an effect's enable step hands back to @ref EffectManager::Toggle to register it. */
struct EffectSetup
{
    uint64_t TimerHandle = 0;       /**< Scheduler handle owned by the effect, or 0. */
    std::function<void()> CancelFn; /**< Undo the effect's state (restore render, team, etc.). */
    bool RoundScoped = false;       /**< Auto-cancel on round end. */
    int DurationMs = 0; /**< >0 auto-cancels the effect after this long; the timer is owned by EffectManager. */
};

class EffectManager
{
public:
    EffectManager() = default;

    bool IsActive(int slot, EffectId id) const;

    /**
     * @brief Register a new effect for `slot`. If an effect of the same id is already
     * active, its cancel callback runs first (re-toggle semantics).
     *
     * @param timerHandle  Scheduler handle owned by the effect. Pass 0 for state-only effects.
     * @param cancelFn     Called when the effect is cancelled for any reason. Should restore
     *                     pawn state and Cancel(timerHandle) itself if applicable.
     * @param roundScoped  True for effects that should auto-cancel on round_end.
     */
    void Apply(int slot, EffectId id, uint64_t timerHandle, std::function<void()> cancelFn, bool roundScoped = false,
               int durationMs = 0);

    /**
     * @brief Toggle an effect. If it is active, cancel it and return false. Otherwise run
     * @p enable (which sets up the effect and returns its @ref EffectSetup), register it, and
     * return true. Lets callers collapse the IsActive/Cancel/Apply dance to one call and pick
     * the broadcast line from the bool: `Broadcast(ctx, on ? "...On" : "...Off")`.
     */
    bool Toggle(int slot, EffectId id, const std::function<EffectSetup()>& enable);

    void Cancel(int slot, EffectId id);
    void CancelAllForSlot(int slot);
    void CancelAllForRoundEnd();
    void CancelAll();

private:
    // Flat 2D array indexed by [slot][effectId]; cheaper than nested maps for
    // <=64 slots and a small enum.
    std::array<std::array<ActiveEffect, static_cast<size_t>(EffectId::Count)>, MaxSlots> _effects{};
};

}  // namespace AdminSystem::Admin::Effects
