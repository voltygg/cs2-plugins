#pragma once

// Plain adapter-to-core data. Keep this header SDK-free for unit tests.

#include <VoltMod/Core/Slot.hpp>
#include <VoltMod/Core/Time.hpp>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>

namespace Anticheat
{

inline constexpr int MaxSlots = VoltMod::Core::MaxPlayers;

inline constexpr bool InSlotRange(int slot)
{
    return VoltMod::Core::IsValidSlot(slot);
}

/**
 * Fixed CS2 simulation rate. CS2 exposes no variable tickrate or interval field;
 * tick-derived thresholds depend on this value.
 */
inline constexpr float TickRate = 64.0f;

/** Time::MonotonicSeconds() drives the rolling evidence windows: real elapsed time, not game
 *  time, and not the wall clock Time::Now() reports. */
using VoltMod::Core::Time;

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

/** Roll lives on CmdSample instead: only AntiAim reads it. */
struct AimAngles
{
    float Pitch = 0.0f;
    float Yaw = 0.0f;
};

/**
 * One decoded usercmd. Aimbot requires adjacent command numbers and client ticks.
 * ServerTick remains -1 until simulation stamps the command.
 */
struct CmdSample
{
    int32_t CmdNum = 0;
    int32_t ClientTick = 0;
    int32_t ServerTick = -1;

    float ViewPitch = 0.0f;
    float ViewYaw = 0.0f;
    float ViewRoll = 0.0f;

    // Summed subtick view deltas for this command.
    float SubtickPitchDelta = 0.0f;
    float SubtickYawDelta = 0.0f;

    /** Where the bullet actually went. Empty when no attack started or the index was capped away. */
    std::optional<AimAngles> AttackAngles;

    bool AttackStarted = false;       // attack1_start_history_index >= 0
    bool AttackIndexInvalid = false;  // index outside [-1, historyTotalCount) - not merely capped away
    bool HasHistoryAngles = false;
    float MaxHistoryYawDelta = 0.0f;  // max |yaw(history entry) - yaw(base)| over the history
    bool BaseAnglesFinite = true;
    bool HistoryAnglesFinite = true;
    bool SubtickAnglesFinite = true;

    // Stamped by ShotCorrelatorCore::OnSimulated.
    Vec3 EyePos;
    bool Airborne = false;

    AimAngles BaseAngles() const { return {ViewPitch, ViewYaw}; }
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
 * A correlated shot: the command that fired it joined to the events it produced. Modules stamp
 * their own consumption flags so one shot funds at most one incident per module.
 */
struct ShotView
{
    uint32_t Generation = 0;
    int Slot = -1;

    int32_t CmdNum = 0;
    int32_t ClientTick = 0;
    int32_t ServerTick = -1;
    int32_t FireTick = -1;

    AimAngles VisibleAngles;  // pawn eye angles at the moment of the fire event
    bool HasVisibleAngles = false;

    Vec3 EyePos;
    Vec3 ImpactPos;
    std::string Weapon;  // normalized (no "weapon_" prefix)

    bool Airborne = false;

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
