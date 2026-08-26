#include "InvalidCvarRules.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <format>

namespace Anticheat
{

static bool EqualsNoCase(std::string_view a, std::string_view b)
{
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
           });
}

/** Whole-string parse: trailing junk makes the value invalid, not merely odd. */
static bool ParseNumber(std::string_view text, double& value)
{
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
        text.remove_prefix(1);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
        text.remove_suffix(1);
    if (text.empty())
        return false;

    const std::string buffer(text);
    char* end = nullptr;
    value = std::strtod(buffer.c_str(), &end);
    return end == buffer.c_str() + buffer.size() && std::isfinite(value);
}

static bool IsOff(std::string_view value, bool numeric, double number)
{
    return EqualsNoCase(value, "false") || (numeric && number == 0.0);
}

static CvarVerdict Invalid(std::string reason, bool kickOnly)
{
    return {.Known = true, .Checked = true, .Invalid = true, .KickOnly = kickOnly, .Reason = std::move(reason)};
}

static CvarVerdict Valid()
{
    return {.Known = true, .Checked = true};
}

static CvarVerdict Skipped()
{
    return {.Known = true, .Checked = false};
}

/**
 * Why @p rule rejects this value, or nullopt when it accepts it. One switch rather than a
 * predicate and a message side by side, which nothing forced to agree.
 */
static std::optional<std::string> Violation(const CvarRule& rule, std::string_view value, bool numeric, double number)
{
    switch (rule.constraint)
    {
    case CvarConstraint::Equals:
        if (number != rule.value)
            return std::format("{} is {:.6g}, but it must be {:.6g}.", rule.name, number, rule.value);
        break;
    case CvarConstraint::Max:
        if (number > rule.value)
            return std::format("{} is {:.6g}, but the maximum allowed value is {:.6g}.", rule.name, number, rule.value);
        break;
    case CvarConstraint::Range:
        if (number < rule.value || number > rule.max)
            return std::format("{} is {:.6g}, but it must be between {:.6g} and {:.6g}.", rule.name, number, rule.value,
                               rule.max);
        break;
    case CvarConstraint::MinOrZero:
        if (number > 0.0 && number < rule.value)
            return std::format("{} is {:.6g}, but it must be at least {:.6g} or 0 for unlimited.", rule.name, number,
                               rule.value);
        break;
    case CvarConstraint::Off:
        if (!IsOff(value, numeric, number))
            return std::format("{} is enabled or invalid on the client while sv_cheats is disabled on the server.",
                               rule.name);
        break;
    case CvarConstraint::On:
        if (IsOff(value, numeric, number))
            return std::format("{} is disabled on the client despite sv_cheats being disabled on the server.",
                               rule.name);
        break;
    }
    return std::nullopt;
}

std::vector<std::string> CvarRuleTable::Load(const std::vector<CvarRule>& rules)
{
    _rules.clear();
    _queriedCount = 0;

    std::vector<std::string> rejected;
    for (CvarTier tier : {CvarTier::Queried, CvarTier::UserInfo})
    {
        for (const CvarRule& rule : rules)
        {
            if (rule.tier != tier)
                continue;
            // Latches are keyed by position, so a second rule for one cvar would share the first
            // one's latch and the two would flip it back and forth against each other.
            if (IndexOf(rule.name) >= 0)
            {
                rejected.push_back(rule.name);
                continue;
            }
            _rules.push_back(rule);
        }
        if (tier == CvarTier::Queried)
            _queriedCount = _rules.size();
    }
    return rejected;
}

int CvarRuleTable::IndexOf(std::string_view name) const
{
    for (size_t index = 0; index < _rules.size(); ++index)
        if (EqualsNoCase(_rules[index].name, name))
            return static_cast<int>(index);
    return -1;
}

size_t CvarRuleTable::PollCvarIndex(size_t cursor, size_t offset) const
{
    return _queriedCount == 0 ? 0 : (cursor + offset) % _queriedCount;
}

CvarVerdict CvarRuleTable::Evaluate(const CvarRule& rule, std::string_view value, bool enforceCheatCvars) const
{
    // Cheat-protected rules only mean something once the server's disabled sv_cheats has certainly
    // reached the client.
    if (rule.cheatProtected && !enforceCheatCvars)
        return Skipped();

    double number = 0.0;
    const bool numeric = ParseNumber(value, number);
    // A value that is not a number at all is never kick-only, however lenient the rule: an
    // out-of-range number can be misconfiguration, but garbage is a client that fabricated a reply.
    if (ConstraintIsNumeric(rule.constraint) && !numeric)
        return Invalid(std::format("{} did not return a valid finite number.", rule.name), false);

    if (std::optional<std::string> why = Violation(rule, value, numeric, number))
        return Invalid(std::move(*why), rule.kickOnly);
    return Valid();
}

CvarVerdict CvarRuleTable::Evaluate(std::string_view name, std::string_view value, bool enforceCheatCvars) const
{
    const int index = IndexOf(name);
    return index < 0 ? CvarVerdict{} : Evaluate(_rules[static_cast<size_t>(index)], value, enforceCheatCvars);
}

CvarVerdict CvarRuleTable::EvaluateMissing(const CvarRule& rule, std::string_view statusName, bool enforceCheatCvars,
                                           int consecutiveReplies) const
{
    if (!rule.cheatProtected || !enforceCheatCvars || consecutiveReplies < MissingRepliesBeforeEvidence)
        return Skipped();
    return Invalid(std::format("{} could not be read from the client on {} replies in a row (latest status {}), "
                               "although every stock client reports it.",
                               rule.name, consecutiveReplies, statusName),
                   true);
}

CvarVerdict CvarRuleTable::EvaluateMissing(std::string_view name, std::string_view statusName, bool enforceCheatCvars,
                                           int consecutiveReplies) const
{
    const int index = IndexOf(name);
    return index < 0
               ? CvarVerdict{}
               : EvaluateMissing(_rules[static_cast<size_t>(index)], statusName, enforceCheatCvars, consecutiveReplies);
}

bool ShouldEnforceCheatCvars(bool svCheatsEnabled, double nowSec, double graceUntilSec)
{
    return !svCheatsEnabled && nowSec > graceUntilSec;
}

void InvalidCvarRules::Reset()
{
    std::ranges::fill(_latched, uint8_t{0});
    std::ranges::fill(_missingReplies, 0);
}

void InvalidCvarRules::OnSlotChanged(int slot)
{
    if (!InSlotRange(slot) || _rules.Size() == 0)
        return;
    const size_t first = At(slot, 0);
    std::fill_n(_latched.begin() + first, _rules.Size(), uint8_t{0});
    std::fill_n(_missingReplies.begin() + first, _rules.Size(), 0);
}

std::vector<std::string> InvalidCvarRules::LoadRules(const std::vector<CvarRule>& rules)
{
    std::vector<std::string> rejected = _rules.Load(rules);
    _latched.assign(static_cast<size_t>(MaxSlots) * _rules.Size(), 0);
    _missingReplies.assign(static_cast<size_t>(MaxSlots) * _rules.Size(), 0);
    return rejected;
}

std::optional<Finding> InvalidCvarRules::Observe(int slot, std::string_view name, std::string_view value,
                                                 bool enforceCheatCvars)
{
    const int index = _rules.IndexOf(name);
    if (!InSlotRange(slot) || index < 0)
        return {};

    const auto at = static_cast<size_t>(index);
    // Any reply carrying a value, valid or not, ends the run of refusals.
    _missingReplies[At(slot, at)] = 0;
    return Apply(slot, at, _rules.Evaluate(_rules.All()[at], value, enforceCheatCvars));
}

std::optional<Finding> InvalidCvarRules::ObserveMissing(int slot, std::string_view name, std::string_view statusName,
                                                        bool enforceCheatCvars)
{
    const int index = _rules.IndexOf(name);
    if (!InSlotRange(slot) || index < 0)
        return {};

    const auto at = static_cast<size_t>(index);
    int& consecutive = _missingReplies[At(slot, at)];
    if (consecutive < MissingRepliesBeforeEvidence)
        ++consecutive;
    return Apply(slot, at, _rules.EvaluateMissing(_rules.All()[at], statusName, enforceCheatCvars, consecutive));
}

std::optional<Finding> InvalidCvarRules::Apply(int slot, size_t index, const CvarVerdict& verdict)
{
    std::optional<Finding> out;
    if (!verdict.Checked)
        return out;

    uint8_t& latched = _latched[At(slot, index)];
    if (!verdict.Invalid)
    {
        latched = 0;
        return out;
    }
    if (latched)
        return out;

    latched = 1;
    out = Finding{.Kind = DetectionKind::InvalidCvar, .KickOnly = verdict.KickOnly, .Evidence = verdict.Reason};
    return out;
}

bool InvalidCvarRules::IsLatched(int slot, std::string_view name) const
{
    const int index = _rules.IndexOf(name);
    return index >= 0 && IsLatchedAt(slot, static_cast<size_t>(index));
}

bool InvalidCvarRules::IsLatchedAt(int slot, size_t index) const
{
    return InSlotRange(slot) && index < _rules.Size() && _latched[At(slot, index)] != 0;
}

}  // namespace Anticheat
