#include "Movement.hpp"

#include "ActionContext.hpp"

#include <mathlib/vector.h>

namespace AdminSystem::Admin::Actions
{

namespace
{
// MoveType_t values from the CS2 schema (see swiftlys2 .../Schemas/Enums/MoveType_t.cs).
// Note: CS2 uses 7 for noclip (8 in CS:GO).
constexpr uint8_t MoveTypeNone = 0;
constexpr uint8_t MoveTypeWalk = 2;
constexpr uint8_t MoveTypeNoclip = 7;

constexpr float BuryDepth = 15.0f;

void SetMoveType(const CS2Kit::Sdk::PlayerController& pc, uint8_t value)
{
    // Both fields must be written or the engine restores the old value on the next tick.
    pc.SetPawnField<uint8_t>("CBaseEntity", "m_MoveType", value);
    pc.SetPawnField<uint8_t>("CBaseEntity", "m_nActualMoveType", value);
}

void ShiftZ(const CS2Kit::Sdk::PlayerController& pc, float deltaZ)
{
    Vector origin = pc.GetAbsOrigin();
    origin.z += deltaZ;
    pc.Teleport(&origin, nullptr, nullptr);
}
}  // namespace

void DoNoclip(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 's');
    if (!ctx.Valid())
        return;

    auto current = ctx.TargetCtrl.GetPawnField<uint8_t>("CBaseEntity", "m_MoveType");
    bool turningOn = (current != MoveTypeNoclip);
    SetMoveType(ctx.TargetCtrl, turningOn ? MoveTypeNoclip : MoveTypeWalk);
    Broadcast(ctx, turningOn ? "broadcast.noclipOn" : "broadcast.noclipOff");
}

void DoFreeze(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 's');
    if (!ctx.Valid())
        return;

    auto current = ctx.TargetCtrl.GetPawnField<uint8_t>("CBaseEntity", "m_MoveType");
    bool turningOn = (current != MoveTypeNone);
    SetMoveType(ctx.TargetCtrl, turningOn ? MoveTypeNone : MoveTypeWalk);
    Broadcast(ctx, turningOn ? "broadcast.freezeOn" : "broadcast.freezeOff");
}

void DoBury(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 's');
    if (!ctx.Valid() || !ctx.TargetCtrl.IsAlive())
        return;
    ShiftZ(ctx.TargetCtrl, -BuryDepth);
    Broadcast(ctx, "broadcast.buried");
}

void DoUnbury(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 's');
    if (!ctx.Valid() || !ctx.TargetCtrl.IsAlive())
        return;
    ShiftZ(ctx.TargetCtrl, BuryDepth);
    Broadcast(ctx, "broadcast.unburied");
}

}  // namespace AdminSystem::Admin::Actions
