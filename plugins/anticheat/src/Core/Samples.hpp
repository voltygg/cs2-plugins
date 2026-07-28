#pragma once

// Plain data the SDK-free cores consume. The adapters fill these from UserCmdView, game events and
// pawn reads; nothing here may depend on the SDK, so the cores stay doctest-able.

#include <chrono>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>

namespace Anticheat
{

/** Slot capacity the cores allocate per-player state for. */
inline constexpr int MaxSlots = 64;

/** True for a slot the cores hold state for; every core guards its per-slot arrays with it. */
inline constexpr bool InSlotRange(int slot)
{
    return slot >= 0 && slot < MaxSlots;
}

/** CS2's fixed simulation rate; every tick-to-seconds conversion in the cores goes through it. */
inline constexpr float TickRate = 64.0f;

/** Monotonic seconds for the cores' rolling evidence windows (wall clock, not game time). */
inline double NowSeconds()
{
    using Clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}

struct Vec3
{
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;

    friend Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.X - b.X, a.Y - b.Y, a.Z - b.Z}; }
    friend Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.X + b.X, a.Y + b.Y, a.Z + b.Z}; }

    float LengthSqr() const { return X * X + Y * Y + Z * Z; }
    float Length() const { return std::sqrt(LengthSqr()); }
};

/** Pitch/yaw pair; roll lives on CmdSample because only AntiAim reads it. */
struct AimAngles
{
    float Pitch = 0.0f;
    float Yaw = 0.0f;
};

/**
 * One decoded usercmd. CmdNum is the client's `legacy_command_number` - adjacency of CmdNum *and*
 * ClientTick is what makes Aimbot's convergence chain trustworthy. ServerTick is -1 until the
 * command is stamped as the one the server actually simulated for a tick.
 */
struct CmdSample
{
    int32_t CmdNum = 0;
    int32_t ClientTick = 0;
    int32_t ServerTick = -1;

    float ViewPitch = 0.0f;
    float ViewYaw = 0.0f;
    float ViewRoll = 0.0f;

    uint64_t ButtonsHeld = 0;
    uint64_t ButtonsChanged = 0;

    int32_t MouseDx = 0;
    int32_t MouseDy = 0;

    // Summed subtick view deltas for this command.
    float SubtickPitchDelta = 0.0f;
    float SubtickYawDelta = 0.0f;

    /** Input-history angles at the attack index: where the bullet actually went. Empty when the
     *  command started no attack or the index was capped away by the history limit. */
    std::optional<AimAngles> AttackAngles;

    bool AttackStarted = false;       // attack1_start_history_index >= 0
    bool AttackIndexInvalid = false;  // index outside [-1, historyTotalCount) - not merely capped away
    bool HasHistoryAngles = false;
    float MaxHistoryYawDelta = 0.0f;  // max |yaw(history entry) - yaw(base)| over the history
    bool BaseAnglesFinite = true;
    bool HistoryAnglesFinite = true;
    bool SubtickAnglesFinite = true;

    // Stamped when the command is simulated (see ShotCorrelatorCore::OnSimulated).
    Vec3 EyePos;
    bool Airborne = false;
    bool Scoped = false;

    AimAngles BaseAngles() const { return {ViewPitch, ViewYaw}; }
    /** The angle the bullet travelled along, falling back to the visible view. */
    AimAngles FiringAngles() const { return AttackAngles ? *AttackAngles : BaseAngles(); }
};

/** One player inside a world snapshot frame. */
struct PositionSample
{
    Vec3 Origin;
    Vec3 EyePos;
    int Team = 0;
    bool Valid = false;
    bool Alive = false;
    bool Teleported = false;  // spawned or teleported within the last 5 seconds
};

/**
 * A correlated shot: the command that fired it joined to the events it produced. Modules read it
 * and stamp their own consumption flags so one shot funds at most one incident per module.
 */
struct ShotView
{
    uint32_t Id = 0;
    uint32_t Generation = 0;
    int Slot = -1;

    int32_t CmdNum = 0;
    int32_t ClientTick = 0;
    int32_t ServerTick = -1;
    int32_t FireTick = -1;

    AimAngles ShotAngles;     // input-history angles the bullet was fired along
    AimAngles VisibleAngles;  // pawn eye angles at the moment of the fire event
    bool HasVisibleAngles = false;

    Vec3 EyePos;
    Vec3 ImpactPos;
    std::string Weapon;  // normalized (no "weapon_" prefix)

    bool Airborne = false;
    bool Scoped = false;

    bool ImpactSeen = false;
    bool HurtSeen = false;
    bool DeathSeen = false;
    int VictimSlot = -1;
    bool Headshot = false;
    bool Wallbang = false;

    // Per-module bookkeeping.
    bool AimbotConsumed = false;
    bool SilentMeasured = false;
    bool SilentConsumed = false;
    float SilentMaxDeviation = 0.0f;
};

}  // namespace Anticheat
