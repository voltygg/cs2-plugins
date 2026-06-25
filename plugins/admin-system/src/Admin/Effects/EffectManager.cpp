#include "EffectManager.hpp"
#include <CS2Kit/Core/Services.hpp>

#include <CS2Kit/Core/Scheduler.hpp>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Effects
{

using CS2Kit::Core::Scheduler;

namespace
{
bool ValidSlot(int slot)
{
    return slot >= 0 && slot < MaxSlots;
}
}  // namespace

bool EffectManager::IsActive(int slot, EffectId id) const
{
    if (!ValidSlot(slot))
        return false;
    return _effects[slot][static_cast<size_t>(id)].Active;
}

void EffectManager::Apply(int slot, EffectId id, uint64_t timerHandle, std::function<void()> cancelFn, bool roundScoped,
                          int durationMs)
{
    if (!ValidSlot(slot))
        return;

    auto& entry = _effects[slot][static_cast<size_t>(id)];
    if (entry.Active)
    {
        if (entry.CancelFn)
            entry.CancelFn();
        if (entry.TimerHandle != 0)
            Engine().Scheduler.Cancel(entry.TimerHandle);
        if (entry.DurationHandle != 0)
            Engine().Scheduler.Cancel(entry.DurationHandle);
    }

    entry.Active = true;
    entry.RoundScoped = roundScoped;
    entry.TimerHandle = timerHandle;
    entry.CancelFn = std::move(cancelFn);
    // Own the auto-expire timer here (not in the effect) so cancelling the effect always cancels it too --
    // a self-scheduled duration timer would survive an early cancel and later clobber a re-applied effect.
    entry.DurationHandle =
        durationMs > 0 ? Engine().Scheduler.Delay(durationMs, [this, slot, id]() { Cancel(slot, id); }) : 0;
}

bool EffectManager::Toggle(int slot, EffectId id, const std::function<EffectSetup()>& enable)
{
    if (!ValidSlot(slot))
        return false;
    if (IsActive(slot, id))
    {
        Cancel(slot, id);
        return false;
    }
    EffectSetup setup = enable();
    Apply(slot, id, setup.TimerHandle, std::move(setup.CancelFn), setup.RoundScoped, setup.DurationMs);
    return true;
}

void EffectManager::Cancel(int slot, EffectId id)
{
    if (!ValidSlot(slot))
        return;
    auto& entry = _effects[slot][static_cast<size_t>(id)];
    if (!entry.Active)
        return;
    if (entry.CancelFn)
        entry.CancelFn();
    if (entry.TimerHandle != 0)
        Engine().Scheduler.Cancel(entry.TimerHandle);
    if (entry.DurationHandle != 0)
        Engine().Scheduler.Cancel(entry.DurationHandle);
    entry = ActiveEffect{};
}

void EffectManager::CancelAllForSlot(int slot)
{
    if (!ValidSlot(slot))
        return;
    for (size_t i = 0; i < static_cast<size_t>(EffectId::Count); ++i)
        Cancel(slot, static_cast<EffectId>(i));
}

void EffectManager::CancelAllForRoundEnd()
{
    for (int slot = 0; slot < MaxSlots; ++slot)
    {
        for (size_t i = 0; i < static_cast<size_t>(EffectId::Count); ++i)
        {
            auto& entry = _effects[slot][i];
            if (entry.Active && entry.RoundScoped)
                Cancel(slot, static_cast<EffectId>(i));
        }
    }
}

void EffectManager::CancelAll()
{
    for (int slot = 0; slot < MaxSlots; ++slot)
        CancelAllForSlot(slot);
}

}  // namespace AdminSystem::Admin::Effects
