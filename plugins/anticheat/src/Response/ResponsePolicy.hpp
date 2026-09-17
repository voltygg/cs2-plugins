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

/** What logs and webhook reports record. @ref Decide returns the first seven; the rest are what
 *  an attempted punishment returned, so nothing reports an enforcement it did not land. */
enum class ResponseOutcome
{
    NoIdentity,       // SteamID not resolved yet - reported, never punished
    Whitelisted,      // reported, never punished
    Observed,         // observe mode
    Alerted,          // alert mode, or ban mode with nothing left to escalate
    AlreadyPunished,  // this player already carries an equal or higher punishment
    KickRequested,    // decided; the execution result replaces it
    BanRequested,     // decided; the execution result replaces it
    KickIssued,       // the client was dropped
    BanIssued,        // admin-system accepted the ban
    TargetGone,       // the player left before the deferred kick ran
    KickFailed,       // the controller refused the kick
    BanUnavailable,   // admin-system is not loaded, so nothing was applied
    BanRejected,      // admin-system refused the ban
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
    case ResponseOutcome::KickRequested:
        return "kick requested";
    case ResponseOutcome::BanRequested:
        return "ban requested";
    case ResponseOutcome::KickIssued:
        return "kicked";
    case ResponseOutcome::BanIssued:
        return "banned";
    case ResponseOutcome::TargetGone:
        return "target gone";
    case ResponseOutcome::KickFailed:
        return "kick failed";
    case ResponseOutcome::BanUnavailable:
        return "ban unavailable";
    case ResponseOutcome::BanRejected:
        return "ban rejected";
    }
    return "observed";
}

/** True for an outcome that means the punishment actually landed. */
constexpr bool Punished(ResponseOutcome outcome)
{
    return outcome == ResponseOutcome::KickIssued || outcome == ResponseOutcome::BanIssued;
}

/** How far the evidence lets the server go, whatever the mode. Certain needs one rule confident
 *  on its own, so fused evidence alerts but never punishes. */
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
    /** What to record when @ref Apply is None. When a punishment is attempted, the executor's
     *  result replaces it: deciding to punish is not the same as having punished. */
    ResponseOutcome Outcome = ResponseOutcome::Observed;
    bool SendAlert = false;
    PunishmentLevel Apply = PunishmentLevel::None;  // None = nothing to issue
};

/** The caller reports every detection regardless; this decides alert and punishment only. */
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

    return {.Outcome =
                requested == PunishmentLevel::Kick ? ResponseOutcome::KickRequested : ResponseOutcome::BanRequested,
            .SendAlert = true,
            .Apply = requested};
}

/** The highest punishment each player actually received; it never goes back down, and rises only
 *  once one lands. Keyed by SteamID, so reconnecting or a map change does not clear it. */
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
