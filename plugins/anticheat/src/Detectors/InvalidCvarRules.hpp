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

/** Poll intervals are randomized within this range. */
inline constexpr float PollIntervalMinSec = 1.0f;
inline constexpr float PollIntervalMaxSec = 5.0f;

/**
 * Cvars per poll. Rotation stays below the framework's per-slot pending cap.
 */
inline constexpr size_t CvarsPerPoll = 4;

/**
 * Refusals required before a cheat-protected cvar becomes evidence. This avoids
 * treating a renamed or removed cvar as a client fault.
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
 * Loaded rules and stateless evaluation. An empty table judges nothing. Queried rules
 * precede userinfo rules so each tier can be exposed as a span.
 */
class CvarRuleTable
{
public:
    /** Load valid rules with queried entries first; return dropped names. Duplicate rules are rejected. */
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
     * Silence is never judged. Refusals are evidence only for cheat-protected rules after
     * @ref MissingRepliesBeforeEvidence consecutive replies, and only for a kick. The rule table
     * contains names supplied by the game, so a refusal differs from an unanswered query.
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
    // Size this from the loaded table; the configuration has no compile-time rule limit.
    std::vector<uint8_t> _latched;
    /** Any reply that does carry a value puts the count back to zero. */
    std::vector<int> _missingReplies;
};

}  // namespace Anticheat
