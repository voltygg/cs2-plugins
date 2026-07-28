#pragma once

// Client settings that are unsafe, impossible, or protected by sv_cheats. SDK-free: the adapter
// owns the querying (userinfo reads and CSVCMsg_GetCvarValue replies) and feeds name/value pairs.

#include "Core/Finding.hpp"
#include "Core/Samples.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace Anticheat
{

/** Cvars queried from the client over the network. */
inline constexpr std::string_view QueriedCvars[] = {
    "fps_max",      "sv_cheats",  "cl_showpos",  "cam_showangles", "cl_drawhud",
    "cl_pitchdown", "cl_pitchup", "cl_yawspeed", "fov_cs_debug",
};

/**
 * Cvars read straight out of the client's userinfo, where they are always readable and never
 * stall. They are deliberately absent from @ref QueriedCvars: two tiers judging one cvar share a
 * single latch and would flip it back and forth against each other.
 */
inline constexpr std::string_view UserInfoCvars[] = {"sensitivity", "m_yaw"};

namespace Detail
{
constexpr auto MakeRuledCvars()
{
    std::array<std::string_view, std::size(QueriedCvars) + std::size(UserInfoCvars)> all{};
    size_t at = 0;
    for (std::string_view name : QueriedCvars)
        all[at++] = name;
    for (std::string_view name : UserInfoCvars)
        all[at++] = name;
    return all;
}
}  // namespace Detail

/** Every cvar the rules cover, queried ones first. Latches are keyed by position in this table. */
inline constexpr auto RuledCvars = Detail::MakeRuledCvars();

/** A replicated sv_cheats change needs time to reach clients before their values mean anything. */
inline constexpr double SvCheatsPropagationGraceSec = 30.0;

/** Polling interval bounds; randomized inside this range so the schedule is not predictable. */
inline constexpr float PollIntervalMinSec = 1.0f;
inline constexpr float PollIntervalMaxSec = 5.0f;

/**
 * Cvars asked for per poll. The table is walked in rotation rather than queried in one burst: the
 * kit refuses a slot's twelfth outstanding query, and the table is nine names long.
 */
inline constexpr size_t CvarsPerPoll = 4;

/**
 * Consecutive replies that refuse a cheat-protected cvar's value before the refusal is evidence.
 * A single one proves nothing: an engine update that renames or drops a client convar would
 * otherwise turn every player on the server into a detection on the very next poll.
 */
inline constexpr int MissingRepliesBeforeEvidence = 3;

/** The @p offset-th cvar of a poll that starts at @p cursor, wrapping over @ref QueriedCvars. */
constexpr size_t PollCvarIndex(size_t cursor, size_t offset)
{
    return (cursor + offset) % std::size(QueriedCvars);
}

/** Delay until a slot's next poll, for a uniform @p unit in [0, 1]. */
constexpr double PollDelaySec(double unit)
{
    return PollIntervalMinSec + std::clamp(unit, 0.0, 1.0) * (PollIntervalMaxSec - PollIntervalMinSec);
}

struct CvarVerdict
{
    bool Known = false;    // the rule table covers this cvar
    bool Checked = false;  // the rule actually ran (cheat-gated rules skip during the grace)
    bool Invalid = false;
    bool KickOnly = false;  // recoverable rules: a kick, never a ban
    std::string Reason;
};

/** Stateless rule evaluation. */
CvarVerdict EvaluateCvar(std::string_view name, std::string_view value, bool enforceCheatCvars);

/**
 * A reply that carried no value at all: the client called the cvar missing, not a cvar, or
 * protected. @p statusName only labels the evidence, and @p consecutiveReplies counts the refusals
 * in a row for this cvar including this one.
 *
 * Silence is never judged here - a client is under no obligation to answer, and an unanswered query
 * simply never arrives. An answer that refuses the value is different: every name in the table ships
 * with the game. Only the cheat-protected ones are treated as evidence, only once
 * @ref MissingRepliesBeforeEvidence of them arrive back to back, and only as a kick, because a
 * client convar the engine removes in a future update would otherwise punish an entire server.
 */
CvarVerdict EvaluateMissingCvar(std::string_view name, std::string_view statusName, bool enforceCheatCvars,
                                int consecutiveReplies);

/**
 * Cheat-protected values may only be enforced while sv_cheats is off and its last disable has had
 * time to propagate. @p graceUntilSec is stamped when sv_cheats goes off (and at load).
 */
bool ShouldEnforceCheatCvars(bool svCheatsEnabled, double nowSec, double graceUntilSec);

/**
 * Per-slot, per-cvar latch over @ref EvaluateCvar: a cvar that stays invalid reports once, and
 * only reports again after it has read valid in between.
 */
class InvalidCvarRules
{
public:
    void Reset();
    void OnSlotChanged(int slot);

    std::optional<Finding> Observe(int slot, std::string_view name, std::string_view value, bool enforceCheatCvars);

    /** @copydoc EvaluateMissingCvar */
    std::optional<Finding> ObserveMissing(int slot, std::string_view name, std::string_view statusName,
                                          bool enforceCheatCvars);

    bool IsLatched(int slot, std::string_view name) const;

private:
    std::optional<Finding> Apply(int slot, std::string_view name, const CvarVerdict& verdict);

    std::array<std::array<bool, std::size(RuledCvars)>, MaxSlots> _latched{};
    /** Refusals in a row per cvar; any reply that does carry a value puts it back to zero. */
    std::array<std::array<int, std::size(RuledCvars)>, MaxSlots> _missingReplies{};
};

}  // namespace Anticheat
