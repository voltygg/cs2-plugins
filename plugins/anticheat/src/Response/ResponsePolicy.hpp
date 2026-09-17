#pragma once

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
enum class ResponseOutcome
{
    NoIdentity,       // SteamID not resolved yet - reported, never punished
    Whitelisted,      // reported, never punished
    Observed,         // observe mode
    Alerted,          // alert mode, or ban mode with nothing left to escalate
    AlreadyPunished,  // this slot already carries an equal or higher punishment
    KickIssued,
    BanIssued,
};

constexpr std::string_view OutcomeName(ResponseOutcome outcome)
{
    switch (outcome)
    {
    case ResponseOutcome::NoIdentity:
        return "no identity";
    case ResponseOutcome::Whitelisted:
        return "whitelisted";
    case ResponseOutcome::Observed:
        return "observed";
    case ResponseOutcome::Alerted:
        return "alerted";
    case ResponseOutcome::AlreadyPunished:
        return "already punished";
    case ResponseOutcome::KickIssued:
        return "kicked";
    case ResponseOutcome::BanIssued:
        return "banned";
    }
    return "observed";
}

struct ResponseInput
{
    int64_t SteamId = 0;
    bool Whitelisted = false;
    Mode CurrentMode = Mode::Observe;
    bool KickOnly = false;                           // from the Finding
    PunishmentLevel Issued = PunishmentLevel::None;  // what this slot already carries
};

struct ResponseDecision
{
    ResponseOutcome Outcome = ResponseOutcome::Observed;
    bool SendAlert = false;
    PunishmentLevel Apply = PunishmentLevel::None;  // None = nothing to issue
};

/** The caller logs and reports every detection regardless; this decides alert and punishment only. */
constexpr ResponseDecision Decide(const ResponseInput& input)
{
    if (input.SteamId == 0)
        return {.Outcome = ResponseOutcome::NoIdentity};
    if (input.Whitelisted)
        return {.Outcome = ResponseOutcome::Whitelisted};
    if (input.CurrentMode == Mode::Observe)
        return {.Outcome = ResponseOutcome::Observed};

    if (input.CurrentMode == Mode::Alert)
        return {.Outcome = ResponseOutcome::Alerted, .SendAlert = true};

    const PunishmentLevel requested = input.KickOnly ? PunishmentLevel::Kick : PunishmentLevel::Ban;
    if (input.Issued >= requested)
        return {.Outcome = ResponseOutcome::AlreadyPunished, .SendAlert = true};

    return {.Outcome = requested == PunishmentLevel::Kick ? ResponseOutcome::KickIssued : ResponseOutcome::BanIssued,
            .SendAlert = true,
            .Apply = requested};
}

/** The highest punishment each slot has already received; it never goes back down. */
class IssuedPunishments
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
