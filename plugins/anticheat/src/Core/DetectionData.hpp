#pragma once

#include <CS2Kit/App/JsonConfig.hpp>

// Game-content facts the detections compare against - Valve's event names and client convars, not
// this plugin's logic. They live in configs/detections.jsonc so a CS2 update that renames a convar
// or adds a HUD event is answered with a config edit and anticheat_reload, not a rebuild.
//
// Parsing is deliberately strict. The generous nlohmann macros map an unrecognized enum token to
// the *first* enumerator and default a missing key silently, which would turn a misspelled
// "minorzero" into "must equal 64" and detect every player on the server. Everything below throws
// instead, so a bad edit fails the load and leaves the last good table in force.
//
// SDK-free (nlohmann only, as Config.hpp is) so the rule engine stays unit-testable.

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace Anticheat
{

inline constexpr const char* DetectionDataPath = "addons/anticheat/configs/detections.jsonc";

/** How the value reaches us. A convar must appear in one tier only: the two share a latch and
 *  would flip it back and forth against each other. */
enum class CvarTier
{
    Queried,   ///< asked over the network with CSVCMsg_GetCvarValue
    UserInfo,  ///< read from the client's userinfo, always readable and never stalling
};

enum class CvarConstraint
{
    Equals,     ///< must equal `value`
    Max,        ///< must not exceed `value`
    Range,      ///< must fall within [`value`, `max`]
    MinOrZero,  ///< must be at least `value`, or 0 for "unlimited"
    Off,        ///< must read as false or 0
    On,         ///< must not read as false or 0
};

/** True when the constraint reads `value` (and, for Range, `max`). */
constexpr bool ConstraintIsNumeric(CvarConstraint constraint)
{
    return constraint != CvarConstraint::Off && constraint != CvarConstraint::On;
}

namespace Detail
{

template <class TEnum, size_t N>
TEnum ParseToken(const nlohmann::json& value, const char* field, const std::pair<std::string_view, TEnum> (&names)[N])
{
    if (!value.is_string())
        throw nlohmann::json::type_error::create(302, std::string(field) + " must be a string", &value);

    const auto text = value.get<std::string>();
    for (const auto& [token, parsed] : names)
        if (token == text)
            return parsed;

    std::string allowed;
    for (const auto& [token, parsed] : names)
        allowed += (allowed.empty() ? "" : ", ") + std::string(token);
    throw nlohmann::json::other_error::create(
        501, std::string(field) + " is \"" + text + "\"; expected one of " + allowed, &value);
}

inline constexpr std::pair<std::string_view, CvarTier> TierNames[] = {
    {"queried", CvarTier::Queried},
    {"userinfo", CvarTier::UserInfo},
};

inline constexpr std::pair<std::string_view, CvarConstraint> ConstraintNames[] = {
    {"equals", CvarConstraint::Equals},       {"max", CvarConstraint::Max}, {"range", CvarConstraint::Range},
    {"minOrZero", CvarConstraint::MinOrZero}, {"off", CvarConstraint::Off}, {"on", CvarConstraint::On},
};

/** Nothing here is optional-with-a-default: a key we do not recognize is a typo, not a preference. */
inline void RequireOnly(const nlohmann::json& object, std::initializer_list<std::string_view> allowed)
{
    for (const auto& [key, unused] : object.items())
    {
        bool known = false;
        for (std::string_view candidate : allowed)
            known = known || candidate == key;
        if (!known)
            throw nlohmann::json::other_error::create(501, "unknown key \"" + key + "\"", &object);
    }
}

}  // namespace Detail

/** Field names are the JSON keys, so they keep their lowercase spelling. */
struct CvarRule
{
    std::string name;
    CvarTier tier = CvarTier::Queried;
    CvarConstraint constraint = CvarConstraint::Equals;
    double value = 0.0;
    double max = 0.0;
    /** Only judged once a disabled sv_cheats has had time to reach the client. */
    bool cheatProtected = false;
    /** Caps the punishment at a kick even in ban mode, for rules whose false-positive cost must
     *  stay recoverable. */
    bool kickOnly = false;
};

inline void from_json(const nlohmann::json& j, CvarRule& rule)
{
    if (!j.is_object())
        throw nlohmann::json::type_error::create(302, "each cvar rule must be an object", &j);
    Detail::RequireOnly(j, {"name", "tier", "constraint", "value", "max", "cheatProtected", "kickOnly"});

    j.at("name").get_to(rule.name);
    if (rule.name.empty())
        throw nlohmann::json::other_error::create(501, "a cvar rule needs a name", &j);

    rule.tier = j.contains("tier") ? Detail::ParseToken(j.at("tier"), "tier", Detail::TierNames) : CvarTier::Queried;
    rule.constraint = Detail::ParseToken(j.at("constraint"), "constraint", Detail::ConstraintNames);

    // A numeric constraint with no bound would silently become "must equal 0" - the likeliest typo
    // in a hand-edited table, and the one that detects everybody.
    if (ConstraintIsNumeric(rule.constraint))
        j.at("value").get_to(rule.value);
    if (rule.constraint == CvarConstraint::Range)
    {
        j.at("max").get_to(rule.max);
        if (rule.max < rule.value)
            throw nlohmann::json::other_error::create(501, "range max is below its value for " + rule.name, &j);
    }

    rule.cheatProtected = j.value("cheatProtected", false);
    rule.kickOnly = j.value("kickOnly", false);
}

struct DetectionData
{
    std::vector<std::string> dllEventBlacklist;
    std::vector<CvarRule> cvarRules;
};

inline void from_json(const nlohmann::json& j, DetectionData& data)
{
    if (!j.is_object())
        throw nlohmann::json::type_error::create(302, "detections must be a JSON object", &j);
    Detail::RequireOnly(j, {"dllEventBlacklist", "cvarRules"});

    // Required rather than defaulted: a renamed section would otherwise load as an empty table and
    // disarm its module while reporting success.
    j.at("dllEventBlacklist").get_to(data.dllEventBlacklist);
    j.at("cvarRules").get_to(data.cvarRules);
}

/** The event/convar tables the detections compare against. */
using DetectionDataManager = CS2Kit::App::JsonConfig<DetectionData>;

}  // namespace Anticheat
