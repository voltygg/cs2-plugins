#pragma once

#include "EffectId.hpp"

#include <CS2Kit/Core/Singleton.hpp>
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
    std::function<void()> CancelFn;
};

class EffectManager : public CS2Kit::Core::Singleton<EffectManager>
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
    void Apply(int slot, EffectId id, uint64_t timerHandle, std::function<void()> cancelFn, bool roundScoped = false);

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
