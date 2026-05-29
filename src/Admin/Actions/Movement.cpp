#include "Movement.hpp"

#include "ActionContext.hpp"

#include <CS2Kit/Sdk/MoveType.hpp>
#include <mathlib/vector.h>

namespace AdminSystem::Admin::Actions
{

using CS2Kit::Sdk::MoveType;

namespace
{
constexpr float BuryDepth = 15.0f;

void ShiftZ(const CS2Kit::Sdk::PlayerController& pc, float deltaZ)
{
    Vector origin = pc.GetAbsOrigin();
    origin.z += deltaZ;
    pc.Teleport(&origin, nullptr, nullptr);
}
}  // namespace

using OptKey = std::optional<std::string>;

void DoNoclip(int adminSlot, int targetSlot)
{
    RunAction(adminSlot, targetSlot, 's', [](const ActionContext& ctx) -> OptKey {
        bool turningOn = (ctx.TargetCtrl.GetMoveType() != MoveType::NoClip);
        ctx.TargetCtrl.SetMoveType(turningOn ? MoveType::NoClip : MoveType::Walk);
        return turningOn ? "broadcast.noclipOn" : "broadcast.noclipOff";
    });
}

void DoFreeze(int adminSlot, int targetSlot)
{
    RunAction(adminSlot, targetSlot, 's', [](const ActionContext& ctx) -> OptKey {
        bool turningOn = (ctx.TargetCtrl.GetMoveType() != MoveType::None);
        ctx.TargetCtrl.SetMoveType(turningOn ? MoveType::None : MoveType::Walk);
        return turningOn ? "broadcast.freezeOn" : "broadcast.freezeOff";
    });
}

void DoBury(int adminSlot, int targetSlot)
{
    RunAction(adminSlot, targetSlot, 's', [](const ActionContext& ctx) -> OptKey {
        if (!ctx.TargetCtrl.IsAlive())
            return std::nullopt;
        ShiftZ(ctx.TargetCtrl, -BuryDepth);
        return "broadcast.buried";
    });
}

void DoUnbury(int adminSlot, int targetSlot)
{
    RunAction(adminSlot, targetSlot, 's', [](const ActionContext& ctx) -> OptKey {
        if (!ctx.TargetCtrl.IsAlive())
            return std::nullopt;
        ShiftZ(ctx.TargetCtrl, BuryDepth);
        return "broadcast.unburied";
    });
}

}  // namespace AdminSystem::Admin::Actions
