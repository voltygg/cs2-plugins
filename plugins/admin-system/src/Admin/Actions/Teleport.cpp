#include "Descriptors.hpp"

#include <cmath>
#include <mathlib/vector.h>
#include <numbers>

namespace AdminSystem::Admin::Actions
{

namespace
{
const Vector ZeroVelocity{0.0f, 0.0f, 0.0f};

// Clearance past the ~32-unit player hull, so a teleported player doesn't clip into the anchor
// and stick (both frozen until one dies).
constexpr float kTeleportClearance = 48.0f;

// Origin `kTeleportClearance` units ahead of `anchor` along its yaw; Z stays at the anchor's level.
Vector ClearedDestination(const CS2Kit::Sdk::PlayerController& anchor)
{
    Vector origin = anchor.GetAbsOrigin();
    float yawRad = anchor.GetEyeAngles().y * std::numbers::pi_v<float> / 180.0f;
    origin.x += std::cos(yawRad) * kTeleportClearance;
    origin.y += std::sin(yawRad) * kTeleportClearance;
    return origin;
}
}  // namespace

const Action Bring{Permission::Control, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                       if (!ctx.AdminCtrl.IsValid())
                           return std::nullopt;
                       Vector dest = ClearedDestination(ctx.AdminCtrl);
                       Vector zero = ZeroVelocity;
                       ctx.TargetCtrl.Teleport(&dest, nullptr, &zero);
                       return "broadcast.brought";
                   }};

const Action Goto{Permission::Control, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      if (!ctx.AdminCtrl.IsValid())
                          return std::nullopt;
                      Vector dest = ClearedDestination(ctx.TargetCtrl);
                      Vector zero = ZeroVelocity;
                      ctx.AdminCtrl.Teleport(&dest, nullptr, &zero);
                      return "broadcast.goto";
                  }};

void Swap(int adminSlot, int firstSlot, int secondSlot)
{
    if (firstSlot == secondSlot)
        return;
    auto ctxA = Resolve(adminSlot, firstSlot, static_cast<char>(Permission::Control));
    auto ctxB = Resolve(adminSlot, secondSlot, static_cast<char>(Permission::Control));
    if (!ctxA.Valid() || !ctxB.Valid())
        return;
    if (!ctxA.TargetCtrl.IsAlive() || !ctxB.TargetCtrl.IsAlive())
        return;

    // Read both destinations before either move, since each reads the other's live origin.
    Vector destForA = ClearedDestination(ctxB.TargetCtrl);
    Vector destForB = ClearedDestination(ctxA.TargetCtrl);
    Vector zero = ZeroVelocity;

    ctxA.TargetCtrl.Teleport(&destForA, nullptr, &zero);
    ctxB.TargetCtrl.Teleport(&destForB, nullptr, &zero);

    Broadcast(ctxA, "broadcast.swapped");
}

}  // namespace AdminSystem::Admin::Actions
