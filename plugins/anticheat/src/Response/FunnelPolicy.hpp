#pragma once

// SDK-free response decisions based on identity, configuration, and prior action.

#include "Core/Samples.hpp"

#include <VoltMod/Core/EnumNames.hpp>
#include <array>
#include <cstdint>
#include <string_view>

namespace Anticheat
{

enum class Mode
{
    Observe,  // Report only.
    Alert,    // Report and notify admins.
    Ban       // Alert, then kick or ban.
};

/** The configured mode, or Observe when the value names no mode. Case-insensitive: a mistyped
 *  "Ban" silently reporting instead of enforcing is not a defensible reading of the setting. */
constexpr Mode ParseMode(std::string_view mode)
{
    return VoltMod::Parse<Mode>(mode).value_or(Mode::Observe);
}

constexpr std::string_view ModeName(Mode mode)
{
    switch (mode)
    {
    case Mode::Observe:
        return "observe";
    case Mode::Alert:
        return "alert";
    case Mode::Ban:
        return "ban";
    }
    return "observe";
}

/** Ordered: a slot's punishment may only ever be raised. */
enum class PunishmentLevel
{
    None = 0,
    Kick = 1,
    Ban = 2,
};

constexpr std::string_view PunishmentName(PunishmentLevel level)
{
    switch (level)
    {
    case PunishmentLevel::None:
        return "none";
    case PunishmentLevel::Kick:
        return "kick";
    case PunishmentLevel::Ban:
        return "ban";
    }
    return "none";
}

/** Outcome recorded in logs and webhook reports. */
enum class FunnelOutcome
{
    NoIdentity,       // SteamID not resolved yet - reported, never punished
    Whitelisted,      // reported, never punished
    Observed,         // observe mode
    Alerted,          // alert mode, or ban mode with nothing left to escalate
    AlreadyPunished,  // this slot already carries an equal or higher punishment
    KickIssued,
    BanIssued,
};

constexpr std::string_view OutcomeName(FunnelOutcome outcome)
{
    switch (outcome)
    {
    case FunnelOutcome::NoIdentity:
        return "no identity";
    case FunnelOutcome::Whitelisted:
        return "whitelisted";
    case FunnelOutcome::Observed:
        return "observed";
    case FunnelOutcome::Alerted:
        return "alerted";
    case FunnelOutcome::AlreadyPunished:
        return "already punished";
    case FunnelOutcome::KickIssued:
        return "kicked";
    case FunnelOutcome::BanIssued:
        return "banned";
    }
    return "observed";
}

struct FunnelInput
{
    int64_t SteamId = 0;
    bool Whitelisted = false;
    Mode CurrentMode = Mode::Observe;
    bool KickOnly = false;                           // from the Finding
    PunishmentLevel Issued = PunishmentLevel::None;  // this slot's latch
};

struct FunnelDecision
{
    FunnelOutcome Outcome = FunnelOutcome::Observed;
    bool SendAlert = false;
    PunishmentLevel Apply = PunishmentLevel::None;  // None = nothing to issue
};

/** The caller logs and reports every detection regardless; this decides alert and punishment only. */
constexpr FunnelDecision Decide(const FunnelInput& input)
{
    if (input.SteamId == 0)
        return {.Outcome = FunnelOutcome::NoIdentity};
    if (input.Whitelisted)
        return {.Outcome = FunnelOutcome::Whitelisted};
    if (input.CurrentMode == Mode::Observe)
        return {.Outcome = FunnelOutcome::Observed};

    if (input.CurrentMode == Mode::Alert)
        return {.Outcome = FunnelOutcome::Alerted, .SendAlert = true};

    const PunishmentLevel requested = input.KickOnly ? PunishmentLevel::Kick : PunishmentLevel::Ban;
    if (input.Issued >= requested)
        return {.Outcome = FunnelOutcome::AlreadyPunished, .SendAlert = true};

    return {.Outcome = requested == PunishmentLevel::Kick ? FunnelOutcome::KickIssued : FunnelOutcome::BanIssued,
            .SendAlert = true,
            .Apply = requested};
}

/** Per-slot no-downgrade record of what has already been done. */
class PunishmentLatch
{
public:
    PunishmentLevel Level(int slot) const
    {
        return InSlotRange(slot) ? _levels[static_cast<size_t>(slot)] : PunishmentLevel::None;
    }

    /** Raises the slot to @p level; false when it was already at or above it. */
    bool Raise(int slot, PunishmentLevel level)
    {
        if (!InSlotRange(slot) || _levels[static_cast<size_t>(slot)] >= level)
            return false;
        _levels[static_cast<size_t>(slot)] = level;
        return true;
    }

    void Clear(int slot)
    {
        if (InSlotRange(slot))
            _levels[static_cast<size_t>(slot)] = PunishmentLevel::None;
    }

    void Reset() { _levels = {}; }

private:
    std::array<PunishmentLevel, MaxSlots> _levels{};
};

}  // namespace Anticheat
