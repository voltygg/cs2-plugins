#include "Core/DetectionData.hpp"

#include <VoltMod/Core/Log.hpp>
#include <format>
#include <string>
#include <utility>

using VoltMod::Error;

namespace Anticheat
{

/** Every check the document's shape cannot state, for one rule. */
static VoltMod::Result<CvarRule> ValidateRule(const DetectionDocument::Rule& raw)
{
    if (raw.name.empty())
        return std::unexpected(Error::Invalid("a cvar rule needs a name"));
    if (!raw.constraint.has_value())
        return std::unexpected(Error::Invalid(std::format("{} has no constraint", raw.name)));

    CvarRule rule{
        .name = raw.name,
        .tier = raw.tier,
        .constraint = *raw.constraint,
        .cheatProtected = raw.cheatProtected,
        .kickOnly = raw.kickOnly,
    };

    // Require the bounds rather than defaulting them: a missing value must not become an
    // implicit zero that quietly changes what the rule means.
    if (ConstraintIsNumeric(rule.constraint))
    {
        if (!raw.value.has_value())
            return std::unexpected(Error::Invalid(std::format("{} has no value", rule.name)));
        rule.value = *raw.value;
    }

    if (rule.constraint == CvarConstraint::Range)
    {
        if (!raw.max.has_value())
            return std::unexpected(Error::Invalid(std::format("{} has no max", rule.name)));
        rule.max = *raw.max;
        if (rule.max < rule.value)
            return std::unexpected(Error::Invalid(std::format("range max is below its value for {}", rule.name)));
    }

    return rule;
}

VoltMod::Result<DetectionData> ValidateDetectionData(DetectionDocument document)
{
    // A renamed section must fail instead of silently disabling its detector, so presence is
    // checked rather than defaulted.
    if (!document.dllEventBlacklist.has_value())
        return std::unexpected(Error::Invalid("no 'dllEventBlacklist' section"));
    if (!document.cvarRules.has_value())
        return std::unexpected(Error::Invalid("no 'cvarRules' section"));

    DetectionData data;
    data.dllEventBlacklist = std::move(*document.dllEventBlacklist);
    data.cvarRules.reserve(document.cvarRules->size());

    for (const auto& raw : *document.cvarRules)
    {
        auto rule = ValidateRule(raw);
        if (!rule)
            return std::unexpected(rule.error());
        data.cvarRules.push_back(std::move(*rule));
    }

    return data;
}

}  // namespace Anticheat
