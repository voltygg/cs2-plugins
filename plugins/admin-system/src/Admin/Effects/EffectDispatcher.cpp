#include "Admin/Effects/EffectDispatcher.hpp"

#include <VoltMod/Entities/EntitySystem.hpp>
#include <utility>

using VoltMod::PlayerRef;

namespace AdminSystem::Admin::Effects
{

void EffectDispatcher::Apply(PlayerRef admin, PlayerRef target, const EffectDescriptor& effect, int param) const
{
    auto ctx = _actions.Resolve(admin, target, effect.Permission);
    if (!ctx)
    {
        return;
    }
    if (effect.RequireAlive && !ctx->Target().Pawn().IsAlive())
    {
        return;
    }
    if (!effect.Setup)
    {
        return;
    }
    if (effect.Choices)
    {
        const int count = static_cast<int>(effect.Choices().size());
        if (param < 0 || param >= count)
        {
            return;
        }
    }

    EffectInstance inst = effect.Setup(*ctx, param);
    // A pure fire-and-forget never occupies the slot map, so IsActive stays false for it.
    const bool hasState = inst.OnTick || inst.OnStop || effect.DurationMs > 0;
    if (hasState)
    {
        _effects.Apply(target.Slot, effect.Id, std::move(inst), effect.Scope, effect.TickIntervalMs, effect.DurationMs);
    }

    if (!effect.OnKey.empty())
    {
        _actions.Broadcast(*ctx, effect.OnKey);
    }
}

void EffectDispatcher::Clear(PlayerRef admin, PlayerRef target, const EffectDescriptor& effect) const
{
    auto ctx = _actions.Resolve(admin, target, effect.Permission);
    if (!ctx || !_effects.IsActive(target.Slot, effect.Id))
    {
        return;
    }

    _effects.Cancel(target.Slot, effect.Id);
    if (!effect.OffKey.empty())
    {
        _actions.Broadcast(*ctx, effect.OffKey);
    }
}

void EffectDispatcher::Toggle(PlayerRef admin, PlayerRef target, const EffectDescriptor& effect, int param) const
{
    if (_effects.IsActive(target.Slot, effect.Id))
    {
        Clear(admin, target, effect);
    }
    else
    {
        Apply(admin, target, effect, param);
    }
}

}  // namespace AdminSystem::Admin::Effects
