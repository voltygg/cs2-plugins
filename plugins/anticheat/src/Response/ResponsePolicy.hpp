#pragma once

#include "Detect/Finding.hpp"

#include <VoltMod/Core/EnumNames.hpp>
#include <cstdint>
#include <string_view>
#include <unordered_map>

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

/** Ordered: a player's punishment may only ever be raised. */
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
    AlreadyPunished,  // this player already carries an equal or higher punishment
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

/**
 * How far the evidence lets the server go, whatever the configured mode.
 *
 * Suspicion reaches Certain only when one rule is confident on its own, so evidence fused from
 * several partial rules can raise an alert but can never get somebody punished by itself.
 */
constexpr Mode CapByConfidence(Mode mode, Confidence level)
{
    switch (level)
    {
    case Confidence::Suspect:
        return Mode::Observe;
    case Confidence::Likely:
        return mode == Mode::Observe ? Mode::Observe : Mode::Alert;
    case Confidence::Certain:
        return mode;
    }
    return Mode::Observe;
}

struct ResponseInput
{
    int64_t SteamId = 0;
    bool Whitelisted = false;
    Mode CurrentMode = Mode::Observe;
    Confidence Level = Confidence::Certain;          // from the Finding
    bool KickOnly = false;                           // from the Finding
    PunishmentLevel Issued = PunishmentLevel::None;  // what this player already carries
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

    const Mode mode = CapByConfidence(input.CurrentMode, input.Level);
    if (mode == Mode::Observe)
        return {.Outcome = ResponseOutcome::Observed};

    if (mode == Mode::Alert)
        return {.Outcome = ResponseOutcome::Alerted, .SendAlert = true};

    const PunishmentLevel requested = input.KickOnly ? PunishmentLevel::Kick : PunishmentLevel::Ban;
    if (input.Issued >= requested)
        return {.Outcome = ResponseOutcome::AlreadyPunished, .SendAlert = true};

    return {.Outcome = requested == PunishmentLevel::Kick ? ResponseOutcome::KickIssued : ResponseOutcome::BanIssued,
            .SendAlert = true,
            .Apply = requested};
}

/**
 * The highest punishment each player has already received; it never goes back down.
 *
 * Keyed by SteamID rather than slot, so reconnecting or sitting out a map change does not hand a
 * kicked player a clean slate. Only players who were kicked or banned are held, so it grows with
 * punishments issued rather than with players seen.
 */
class IssuedPunishments
{
public:
    PunishmentLevel Level(int64_t steamId) const
    {
        const auto it = _levels.find(steamId);
        return it == _levels.end() ? PunishmentLevel::None : it->second;
    }

    /** Raises @p steamId to @p level; false when it was already at or above it. */
    bool Raise(int64_t steamId, PunishmentLevel level)
    {
        PunishmentLevel& held = _levels[steamId];
        if (held >= level)
            return false;
        held = level;
        return true;
    }

    void Reset() { _levels.clear(); }

private:
    std::unordered_map<int64_t, PunishmentLevel> _levels;
};

}  // namespace Anticheat
