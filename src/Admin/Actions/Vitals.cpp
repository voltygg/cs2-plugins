#include "Vitals.hpp"

#include "ActionContext.hpp"

#include <CS2Kit/Sdk/Entity.hpp>

namespace AdminSystem::Admin::Actions
{

void DoSlay(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 's');
    if (!ctx.Valid() || !ctx.TargetCtrl.IsAlive())
    {
        return;
    }

    ctx.TargetCtrl.Slay();
    Broadcast(ctx, "broadcast.slain");
}

void DoSetHealth(int adminSlot, int targetSlot, int health)
{
    auto ctx = Resolve(adminSlot, targetSlot, 'h');
    if (!ctx.Valid() || !ctx.TargetCtrl.IsAlive())
    {
        return;
    }

    ctx.TargetCtrl.SetPawnField<int>("CBaseEntity", "m_iHealth", health);
    Broadcast(ctx, "broadcast.healed");
}

void DoSetArmor(int adminSlot, int targetSlot, int armor)
{
    auto ctx = Resolve(adminSlot, targetSlot, 'h');
    if (!ctx.Valid() || !ctx.TargetCtrl.IsAlive())
    {
        return;
    }

    ctx.TargetCtrl.SetPawnField<int>("CCSPlayerPawn", "m_ArmorValue", armor);
    Broadcast(ctx, "broadcast.armored");
}

void DoToggleGodmode(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 'h');
    if (!ctx.Valid() || !ctx.TargetCtrl.IsAlive())
    {
        return;
    }

    auto flags = ctx.TargetCtrl.GetPawnField<uint32_t>("CBaseEntity", "m_fFlags");
    bool turningOn = (flags & CS2Kit::Sdk::FL_GODMODE) == 0;
    uint32_t newFlags = turningOn ? (flags | CS2Kit::Sdk::FL_GODMODE) : (flags & ~CS2Kit::Sdk::FL_GODMODE);
    ctx.TargetCtrl.SetPawnField<uint32_t>("CBaseEntity", "m_fFlags", newFlags);
    Broadcast(ctx, turningOn ? "broadcast.godmodeOn" : "broadcast.godmodeOff");
}

}  // namespace AdminSystem::Admin::Actions
