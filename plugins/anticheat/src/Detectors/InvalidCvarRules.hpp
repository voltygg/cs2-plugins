#pragma once

// SDK-free evaluation of client convars supplied by the engine adapter.

#include "Core/DetectionData.hpp"
#include "Core/Finding.hpp"
#include "Core/Samples.hpp"

#include <algorithm>
#include <optional>
#include <span>
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
 * Cvars per poll. Rotation stays below the framework's per-slot pending cap.
 */
inline constexpr size_t CvarsPerPoll = 4;

/**
 * Consecutive refusals required before a cheat-protected cvar becomes evidence.
 * This prevents a renamed or removed
 * cvar from detecting every client at once.
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
 * Loaded rules and stateless evaluation. An empty table judges nothing.
 *
 * Rules are stored queried tier first, so each tier is a span rather than a second container to
 * keep in step.
 */
class CvarRuleTable
{
public:
    /** Keeps the rules that validate, queried tier first; returns the names it dropped. Duplicates
     *  are rejected, since a second rule for one cvar would share the first one's latch. */
    std::vector<std::string> Load(const std::vector<CvarRule>& rules);

    size_t Size() const { return _rules.size(); }
    std::span<const CvarRule> All() const { return _rules; }

    /** The poll rotation walks these; the userinfo tier is read directly every poll. */
    std::span<const CvarRule> Queried() const { return All().first(_queriedCount); }
    std::span<const CvarRule> UserInfo() const { return All().subspan(_queriedCount); }

    /** Position in the table, which is what latches are keyed by, or -1 when not covered.
     *  Matched without regard to case, as the engine spells convars inconsistently. */
    int IndexOf(std::string_view name) const;

    /** The @p offset-th cvar of a poll starting at @p cursor, wrapping over the queried tier. */
    size_t PollCvarIndex(size_t cursor, size_t offset) const;

    CvarVerdict Evaluate(std::string_view name, std::string_view value, bool enforceCheatCvars) const;

    /**
     * Evaluate a reply that refused to return a value. @p consecutiveReplies
     * includes the current refusal.
 *
     * Silence is never judged - an unanswered query simply never arrives. A refusal is different,
     * since every name in the table ships with the game, but it is still only evidence for the
     * cheat-protected ones, only after @ref MissingRepliesBeforeEvidence back to back, and only as
     * a kick.
     */
    CvarVerdict EvaluateMissing(std::string_view name, std::string_view statusName, bool enforceCheatCvars,
                                int consecutiveReplies) const;

    /** As @ref Evaluate, for a rule the caller has already resolved. */
    CvarVerdict Evaluate(const CvarRule& rule, std::string_view value, bool enforceCheatCvars) const;
    CvarVerdict EvaluateMissing(const CvarRule& rule, std::string_view statusName, bool enforceCheatCvars,
                                int consecutiveReplies) const;

private:
    std::vector<CvarRule> _rules;
    size_t _queriedCount = 0;
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

    /** Replaces the rules and drops every latch, since latches are keyed by table position.
     *  Returns the names of any rules that did not validate. */
    std::vector<std::string> LoadRules(const std::vector<CvarRule>& rules);

    const CvarRuleTable& Rules() const { return _rules; }

    std::optional<Finding> Observe(int slot, std::string_view name, std::string_view value, bool enforceCheatCvars);

    /** @copydoc CvarRuleTable::EvaluateMissing */
    std::optional<Finding> ObserveMissing(int slot, std::string_view name, std::string_view statusName,
                                          bool enforceCheatCvars);

    bool IsLatched(int slot, std::string_view name) const;
    /** For callers already walking the table, which know the position. */
    bool IsLatchedAt(int slot, size_t index) const;

private:
    std::optional<Finding> Apply(int slot, size_t index, const CvarVerdict& verdict);
    size_t At(int slot, size_t index) const { return static_cast<size_t>(slot) * _rules.Size() + index; }

    CvarRuleTable _rules;
    // Sized MaxSlots * rule count at load: the table is runtime data, so a compile-time cap would
    // only put a second limit on what the file may contain.
    std::vector<uint8_t> _latched;
    /** Any reply that does carry a value puts the count back to zero. */
    std::vector<int> _missingReplies;
};

}  // namespace Anticheat
