#pragma once

// Client settings that are unsafe, impossible, or protected by sv_cheats. SDK-free: the adapter
// owns the querying (userinfo reads and CSVCMsg_GetCvarValue replies) and feeds name/value pairs,
// and the rules themselves come from configs/detections.jsonc rather than from this file.

#include "Core/DetectionData.hpp"
#include "Core/Finding.hpp"
#include "Core/Samples.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Anticheat
{

/** A replicated sv_cheats change needs time to reach clients before their values mean anything. */
inline constexpr double SvCheatsPropagationGraceSec = 30.0;

/** Randomized inside this range so the schedule is not predictable. */
inline constexpr float PollIntervalMinSec = 1.0f;
inline constexpr float PollIntervalMaxSec = 5.0f;

/**
 * Cvars asked for per poll. The queried tier is walked in rotation rather than queried in one
 * burst: the kit refuses a slot's twelfth outstanding query.
 */
inline constexpr size_t CvarsPerPoll = 4;

/**
 * Consecutive replies refusing a cheat-protected cvar's value before the refusal is evidence. A
 * single one proves nothing: an engine update that renames or drops a client convar would otherwise
 * turn every player on the server into a detection on the very next poll.
 */
inline constexpr int MissingRepliesBeforeEvidence = 3;

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

/**
 * The loaded rules, and the stateless evaluation over them. Empty until Load succeeds, and an empty
 * table judges nothing - a missing or malformed data file leaves the module inert rather than
 * turning every client value into a detection.
 */
class CvarRuleTable
{
public:
    /** Keeps the rules that validate, in file order; returns how many were rejected. Duplicate
     *  names and the two-tiers-one-cvar case are rejected, since latches are keyed by name. */
    int Load(const std::vector<CvarRule>& rules);

    void Clear();

    bool Empty() const { return _rules.empty(); }
    size_t Size() const { return _rules.size(); }
    const std::vector<CvarRule>& All() const { return _rules; }

    /** Matched without regard to case, as the engine spells convars inconsistently. */
    const CvarRule* Find(std::string_view name) const;

    /** Position in the table, which is what latches are keyed by, or -1 when not covered. */
    int IndexOf(std::string_view name) const;

    /** Names of the queried tier, in table order; the poll rotation walks these. */
    const std::vector<std::string_view>& Queried() const { return _queried; }
    const std::vector<std::string_view>& UserInfo() const { return _userInfo; }

    /** The @p offset-th cvar of a poll starting at @p cursor, wrapping over the queried tier. */
    size_t PollCvarIndex(size_t cursor, size_t offset) const;

    CvarVerdict Evaluate(std::string_view name, std::string_view value, bool enforceCheatCvars) const;

    /**
     * A reply that carried no value at all: the client called the cvar missing, not a cvar, or
     * protected. @p statusName only labels the evidence; @p consecutiveReplies counts the refusals
     * in a row for this cvar, including this one.
     *
     * Silence is never judged - an unanswered query simply never arrives. A refusal is different,
     * since every name in the table ships with the game, but it is still only evidence for the
     * cheat-protected ones, only after @ref MissingRepliesBeforeEvidence back to back, and only as
     * a kick.
     */
    CvarVerdict EvaluateMissing(std::string_view name, std::string_view statusName, bool enforceCheatCvars,
                                int consecutiveReplies) const;

private:
    std::vector<CvarRule> _rules;
    std::vector<std::string_view> _queried;   // views into _rules, rebuilt on every Load
    std::vector<std::string_view> _userInfo;  // same
};

/**
 * Cheat-protected values may only be enforced while sv_cheats is off and its last disable has had
 * time to propagate. @p graceUntilSec is stamped when sv_cheats goes off (and at load).
 */
bool ShouldEnforceCheatCvars(bool svCheatsEnabled, double nowSec, double graceUntilSec);

/**
 * Per-slot, per-cvar latch over @ref CvarRuleTable: a cvar that stays invalid reports once, and
 * only again after it has read valid in between.
 */
class InvalidCvarRules
{
public:
    void Reset();
    void OnSlotChanged(int slot);

    /** Replaces the rules and drops every latch, since latches are keyed by table position. */
    int LoadRules(const std::vector<CvarRule>& rules);

    const CvarRuleTable& Rules() const { return _rules; }

    std::optional<Finding> Observe(int slot, std::string_view name, std::string_view value, bool enforceCheatCvars);

    /** @copydoc CvarRuleTable::EvaluateMissing */
    std::optional<Finding> ObserveMissing(int slot, std::string_view name, std::string_view statusName,
                                          bool enforceCheatCvars);

    bool IsLatched(int slot, std::string_view name) const;

private:
    std::optional<Finding> Apply(int slot, std::string_view name, const CvarVerdict& verdict);

    CvarRuleTable _rules;
    std::array<std::array<bool, MaxRuledCvars>, MaxSlots> _latched{};
    /** Any reply that does carry a value puts the count back to zero. */
    std::array<std::array<int, MaxRuledCvars>, MaxSlots> _missingReplies{};
};

}  // namespace Anticheat
