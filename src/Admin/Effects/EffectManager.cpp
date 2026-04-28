#include "EffectManager.hpp"

#include <CS2Kit/Core/Scheduler.hpp>

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

void EffectManager::Apply(int slot, EffectId id, uint64_t timerHandle, std::function<void()> cancelFn, bool roundScoped)
{
    if (!ValidSlot(slot))
        return;

    auto& entry = _effects[slot][static_cast<size_t>(id)];
    if (entry.Active && entry.CancelFn)
        entry.CancelFn();

    entry.Active = true;
    entry.RoundScoped = roundScoped;
    entry.TimerHandle = timerHandle;
    entry.CancelFn = std::move(cancelFn);
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
        Scheduler::Instance().Cancel(entry.TimerHandle);
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
