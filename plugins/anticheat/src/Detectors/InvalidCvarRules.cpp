#include "InvalidCvarRules.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <format>

namespace Anticheat
{

namespace
{
bool EqualsNoCase(std::string_view a, std::string_view b)
{
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
           });
}

/** Whole-string parse: trailing junk makes the value invalid, not merely odd. */
bool ParseNumber(std::string_view text, double& value)
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

bool IsOff(std::string_view value, bool numeric, double number)
{
    return EqualsNoCase(value, "false") || (numeric && number == 0.0);
}

CvarVerdict Invalid(std::string reason, bool kickOnly)
{
    return {.Known = true, .Checked = true, .Invalid = true, .KickOnly = kickOnly, .Reason = std::move(reason)};
}

CvarVerdict Valid()
{
    return {.Known = true, .Checked = true};
}

CvarVerdict Skipped()
{
    return {.Known = true, .Checked = false};
}

/** Numeric constraints only: the on/off pair reads words as well as numbers. */
bool NeedsNumber(CvarConstraint constraint)
{
    return constraint != CvarConstraint::Off && constraint != CvarConstraint::On;
}

bool Satisfies(const CvarRule& rule, std::string_view value, bool numeric, double number)
{
    switch (rule.constraint)
    {
    case CvarConstraint::Equals:
        return number == rule.value;
    case CvarConstraint::Max:
        return number <= rule.value;
    case CvarConstraint::Range:
        return number >= rule.value && number <= rule.max;
    case CvarConstraint::MinOrZero:
        return number <= 0.0 || number >= rule.value;
    case CvarConstraint::Off:
        return IsOff(value, numeric, number);
    case CvarConstraint::On:
        return !IsOff(value, numeric, number);
    }
    return true;
}

std::string Explain(const CvarRule& rule, double number)
{
    switch (rule.constraint)
    {
    case CvarConstraint::Equals:
        return std::format("{} is {:.6g}, but it must be {:.6g}.", rule.name, number, rule.value);
    case CvarConstraint::Max:
        return std::format("{} is {:.6g}, but the maximum allowed value is {:.6g}.", rule.name, number, rule.value);
    case CvarConstraint::Range:
        return std::format("{} is {:.6g}, but it must be between {:.6g} and {:.6g}.", rule.name, number, rule.value,
                           rule.max);
    case CvarConstraint::MinOrZero:
        return std::format("{} is {:.6g}, but it must be at least {:.6g} or 0 for unlimited.", rule.name, number,
                           rule.value);
    case CvarConstraint::Off:
        return std::format("{} is enabled or invalid on the client while sv_cheats is disabled on the server.",
                           rule.name);
    case CvarConstraint::On:
        return std::format("{} is disabled on the client despite sv_cheats being disabled on the server.", rule.name);
    }
    return std::format("{} holds a value the server does not allow.", rule.name);
}

bool RuleIsSane(const CvarRule& rule)
{
    if (rule.name.empty())
        return false;
    if (rule.constraint == CvarConstraint::Range && rule.max < rule.value)
        return false;
    return true;
}
}  // namespace

int CvarRuleTable::Load(const std::vector<CvarRule>& rules)
{
    Clear();

    int rejected = 0;
    for (const CvarRule& rule : rules)
    {
        // Latches are keyed by name, so a second rule for one cvar would share a latch with the
        // first and the two would flip it back and forth against each other.
        if (!RuleIsSane(rule) || Find(rule.name) != nullptr || _rules.size() >= MaxRuledCvars)
        {
            ++rejected;
            continue;
        }
        _rules.push_back(rule);
    }

    for (const CvarRule& rule : _rules)
        (rule.tier == CvarTier::Queried ? _queried : _userInfo).push_back(rule.name);
    return rejected;
}

void CvarRuleTable::Clear()
{
    _rules.clear();
    _queried.clear();
    _userInfo.clear();
}

const CvarRule* CvarRuleTable::Find(std::string_view name) const
{
    auto found = std::ranges::find_if(_rules, [&](const CvarRule& rule) { return EqualsNoCase(rule.name, name); });
    return found == _rules.end() ? nullptr : &*found;
}

int CvarRuleTable::IndexOf(std::string_view name) const
{
    const CvarRule* rule = Find(name);
    return rule ? static_cast<int>(rule - _rules.data()) : -1;
}

size_t CvarRuleTable::PollCvarIndex(size_t cursor, size_t offset) const
{
    return _queried.empty() ? 0 : (cursor + offset) % _queried.size();
}

CvarVerdict CvarRuleTable::Evaluate(std::string_view name, std::string_view value, bool enforceCheatCvars) const
{
    const CvarRule* rule = Find(name);
    if (!rule)
        return {};

    // Cheat-protected rules only mean something once the server's disabled sv_cheats has certainly
    // reached the client.
    if (rule->cheatProtected && !enforceCheatCvars)
        return Skipped();

    double number = 0.0;
    const bool numeric = ParseNumber(value, number);
    // A value that is not a number at all is never kick-only, however lenient the rule: an
    // out-of-range number can be misconfiguration, but garbage is a client that fabricated a reply.
    if (NeedsNumber(rule->constraint) && !numeric)
        return Invalid(std::format("{} did not return a valid finite number.", rule->name), false);

    if (Satisfies(*rule, value, numeric, number))
        return Valid();
    return Invalid(Explain(*rule, number), rule->kickOnly);
}

CvarVerdict CvarRuleTable::EvaluateMissing(std::string_view name, std::string_view statusName, bool enforceCheatCvars,
                                           int consecutiveReplies) const
{
    const CvarRule* rule = Find(name);
    if (!rule)
        return {};
    if (!rule->cheatProtected || !enforceCheatCvars || consecutiveReplies < MissingRepliesBeforeEvidence)
        return Skipped();
    return Invalid(std::format("{} could not be read from the client on {} replies in a row (latest status {}), "
                               "although every stock client reports it.",
                               rule->name, consecutiveReplies, statusName),
                   true);
}

bool ShouldEnforceCheatCvars(bool svCheatsEnabled, double nowSec, double graceUntilSec)
{
    return !svCheatsEnabled && nowSec > graceUntilSec;
}

void InvalidCvarRules::Reset()
{
    _latched = {};
    _missingReplies = {};
}

void InvalidCvarRules::OnSlotChanged(int slot)
{
    if (!InSlotRange(slot))
        return;
    _latched[slot] = {};
    _missingReplies[slot] = {};
}

int InvalidCvarRules::LoadRules(const std::vector<CvarRule>& rules)
{
    const int rejected = _rules.Load(rules);
    Reset();
    return rejected;
}

std::optional<Finding> InvalidCvarRules::Observe(int slot, std::string_view name, std::string_view value,
                                                 bool enforceCheatCvars)
{
    // Any reply carrying a value, valid or not, ends the run of refusals.
    if (const int index = _rules.IndexOf(name); InSlotRange(slot) && index >= 0)
        _missingReplies[slot][static_cast<size_t>(index)] = 0;
    return Apply(slot, name, _rules.Evaluate(name, value, enforceCheatCvars));
}

std::optional<Finding> InvalidCvarRules::ObserveMissing(int slot, std::string_view name, std::string_view statusName,
                                                        bool enforceCheatCvars)
{
    const int index = _rules.IndexOf(name);
    if (!InSlotRange(slot) || index < 0)
        return {};

    int& consecutive = _missingReplies[slot][static_cast<size_t>(index)];
    if (consecutive < MissingRepliesBeforeEvidence)
        ++consecutive;
    return Apply(slot, name, _rules.EvaluateMissing(name, statusName, enforceCheatCvars, consecutive));
}

std::optional<Finding> InvalidCvarRules::Apply(int slot, std::string_view name, const CvarVerdict& verdict)
{
    std::optional<Finding> out;
    const int index = _rules.IndexOf(name);
    if (!InSlotRange(slot) || index < 0 || !verdict.Checked)
        return out;

    bool& latched = _latched[slot][static_cast<size_t>(index)];
    if (!verdict.Invalid)
    {
        latched = false;
        return out;
    }
    if (latched)
        return out;

    latched = true;
    out = Finding{.Kind = DetectionKind::InvalidCvar, .KickOnly = verdict.KickOnly, .Evidence = verdict.Reason};
    return out;
}

bool InvalidCvarRules::IsLatched(int slot, std::string_view name) const
{
    const int index = _rules.IndexOf(name);
    return InSlotRange(slot) && index >= 0 && _latched[slot][static_cast<size_t>(index)];
}

}  // namespace Anticheat
