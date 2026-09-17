#pragma once

#include <VoltMod/Core/Slot.hpp>
#include <VoltMod/Core/Time.hpp>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>

namespace Anticheat
{

inline constexpr int MaxSlots = VoltMod::MaxPlayers;
static_assert(MaxSlots <= 64, "slot bitmasks below are 64 bits wide");

inline constexpr bool InSlotRange(int slot)
{
    return VoltMod::IsValidSlot(slot);
}

/** One bit per slot, for who-sees-whom and similar per-pair facts. */
inline constexpr uint64_t SlotBit(int slot)
{
    return InSlotRange(slot) ? uint64_t{1} << slot : 0;
}

/**
 * Fixed CS2 simulation rate. CS2 exposes no variable tickrate or interval field;
 * tick-derived thresholds depend on this value.
 */
inline constexpr float TickRate = 64.0f;

/** Time::MonotonicSeconds() drives the rolling evidence windows: real elapsed time, not game
 *  time, and not the wall clock Time::Now() reports. */

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

/** The usercmd buttons the cores read. Values match the engine's IN_* bits. */
inline constexpr uint64_t ButtonAttack = 0x1;
inline constexpr uint64_t ButtonTurnLeft = 0x80;
inline constexpr uint64_t ButtonTurnRight = 0x100;

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

    /** Raw mouse counts the client reports for this command. */
    int32_t MouseDx = 0;
    int32_t MouseDy = 0;
    uint64_t Buttons = 0;

    /** Where the bullet actually went. Empty when no attack started or the index was capped away. */
    std::optional<AimAngles> AttackAngles;

    bool AttackStarted = false;       // attack1_start_history_index >= 0
    bool AttackIndexInvalid = false;  // index outside [-1, historyTotalCount) - not merely capped away
    bool HasHistoryAngles = false;
    float MaxHistoryYawDelta = 0.0f;  // max |yaw(history entry) - yaw(base)| over the history
    bool BaseAnglesFinite = true;
    bool HistoryAnglesFinite = true;
    bool SubtickAnglesFinite = true;

    // Pawn state read as the command arrives, before the engine simulates it.
    Vec3 EyePos;
    bool Airborne = false;
    bool Scoped = false;
    /** The recoil punch the last shot left behind, as the client predicted it for this command. */
    AimAngles Punch;
    bool HasPunch = false;

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

    /** Sight lines are traced only for the pairs worth asking about, so a viewer's bit in
     *  CheckedBy says whether SeenBy holds an answer for it at all. */
    uint64_t CheckedBy = 0;
    uint64_t SeenBy = 0;

    /** A real player in this frame: the engine gave us a pawn, and it is alive. */
    bool InPlay() const { return Valid && Alive; }

    /** In play and not warped by a recent teleport, so its motion reads across frames. */
    bool Trackable() const { return InPlay() && !Teleported; }

    bool SightKnownTo(int viewer) const { return (CheckedBy & SlotBit(viewer)) != 0; }
    bool VisibleTo(int viewer) const { return (SeenBy & SlotBit(viewer)) != 0; }
    bool HiddenFrom(int viewer) const { return SightKnownTo(viewer) && !VisibleTo(viewer); }
};

/**
 * A correlated shot: the command that fired it joined to the events it produced. Every shot is
 * finalized once, after its events have had time to arrive, and each core reads it then.
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
    /** The burst index the pawn reported at the fire event; 1 for the first shot of a spray. */
    int ShotsFired = 0;

    bool ImpactSeen = false;
    bool HurtSeen = false;
    bool DeathSeen = false;
    int VictimSlot = -1;
    bool Headshot = false;
    bool Wallbang = false;

    // Per-module bookkeeping.
    bool AimbotConsumed = false;
    bool SilentMeasured = false;
    float SilentMaxDeviation = 0.0f;
    bool Finalized = false;
};

}  // namespace Anticheat
