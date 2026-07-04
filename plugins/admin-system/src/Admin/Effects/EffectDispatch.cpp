#include "../../Core/Managers.hpp"
#include "EffectDescriptor.hpp"

#include <utility>

namespace AdminSystem::Admin::Effects
{

namespace
{

// Build the kit EffectSpec from the descriptor's declarative lifetime plus the body's instance.
CS2Kit::Core::EffectSpec MakeSpec(EffectScope scope, int tickIntervalMs, int durationMs, EffectInstance inst)
{
    return {.TickIntervalMs = tickIntervalMs,
            .DurationMs = durationMs,
            .RoundScoped = scope == EffectScope::Round,
            .OnTick = std::move(inst.OnTick),
            .OnStop = std::move(inst.OnStop)};
}

// Shared body for the Effect/ParamEffect Clear verbs (both key off Flag/Id/OffKey only).
void ClearById(int adminSlot, int targetSlot, const std::string& flag, int id, const std::string& offKey)
{
    auto ctx = Actions::Resolve(adminSlot, targetSlot, flag);
    if (!ctx.Valid() || !App().Effects.IsActive(targetSlot, id))
        return;

    App().Effects.Cancel(targetSlot, id);
    Actions::Broadcast(ctx, offKey);
}

}  // namespace

void Apply(int adminSlot, int targetSlot, const Effect& effect)
{
    auto ctx = Actions::Resolve(adminSlot, targetSlot, effect.Flag);
    if (!ctx.Valid())
        return;
    if (effect.RequireAlive && !ctx.TargetCtrl.IsAlive())
        return;

    EffectInstance inst = effect.Setup ? effect.Setup(ctx) : EffectInstance{};
    // Register only when there is state to track: a pure fire-and-forget never occupies the slot
    // map, so IsActive stays false and no stale toggle state lingers.
    if (inst.OnTick || inst.OnStop || effect.DurationMs > 0)
        App().Effects.Apply(targetSlot, effect.Id,
                            MakeSpec(effect.Scope, effect.TickIntervalMs, effect.DurationMs, std::move(inst)));

    Actions::Broadcast(ctx, effect.OnKey);
}

void Clear(int adminSlot, int targetSlot, const Effect& effect)
{
    ClearById(adminSlot, targetSlot, effect.Flag, effect.Id, effect.OffKey);
}

void Toggle(int adminSlot, int targetSlot, const Effect& effect)
{
    if (App().Effects.IsActive(targetSlot, effect.Id))
        Clear(adminSlot, targetSlot, effect);
    else
        Apply(adminSlot, targetSlot, effect);
}

void Apply(int adminSlot, int targetSlot, int param, const ParamEffect& effect)
{
    auto ctx = Actions::Resolve(adminSlot, targetSlot, effect.Flag);
    if (!ctx.Valid() || !effect.Setup)
        return;
    if (effect.RequireAlive && !ctx.TargetCtrl.IsAlive())
        return;
    if (param < 0 || (effect.Choices && param >= static_cast<int>(effect.Choices().size())))
        return;

    EffectInstance inst = effect.Setup(ctx, param);
    App().Effects.Apply(targetSlot, effect.Id,
                        MakeSpec(effect.Scope, effect.TickIntervalMs, effect.DurationMs, std::move(inst)));
    Actions::Broadcast(ctx, effect.OnKey);
}

void Clear(int adminSlot, int targetSlot, const ParamEffect& effect)
{
    ClearById(adminSlot, targetSlot, effect.Flag, effect.Id, effect.OffKey);
}

}  // namespace AdminSystem::Admin::Effects
