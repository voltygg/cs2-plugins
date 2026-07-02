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

    // Exact origins, unlike Bring/Goto: both spots are vacated in the same frame, so there is
    // nothing to clear past - and a facing offset lets the two destinations converge into the
    // very collision stick it was meant to avoid when the players face each other.
    Vector posA = ctxA.TargetCtrl.GetAbsOrigin();
    Vector posB = ctxB.TargetCtrl.GetAbsOrigin();
    Vector zero = ZeroVelocity;

    ctxA.TargetCtrl.Teleport(&posB, nullptr, &zero);
    ctxB.TargetCtrl.Teleport(&posA, nullptr, &zero);

    Broadcast(ctxA, ctxB, "broadcast.swapped");
}

}  // namespace AdminSystem::Admin::Actions
