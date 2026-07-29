#pragma once

// Game-content facts the detections compare against - Valve's event names and client convars, not
// this plugin's logic. They live in configs/detections.jsonc so a CS2 update that renames a convar
// or adds a HUD event is answered with a config edit and anticheat_reload, not a rebuild.
//
// SDK-free (nlohmann only, as Config.hpp is) so the rule engine stays unit-testable.

#include <cstddef>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Anticheat
{

inline constexpr const char* DetectionDataPath = "addons/anticheat/configs/detections.jsonc";

/** Latches are keyed by position in the rule table, so the table is bounded rather than dynamic. */
inline constexpr size_t MaxRuledCvars = 32;

/** How the value reaches us. The two tiers must not both cover one cvar: they share a latch and
 *  would flip it back and forth against each other. */
enum class CvarTier
{
    Queried,   ///< asked over the network with CSVCMsg_GetCvarValue
    UserInfo,  ///< read from the client's userinfo, always readable and never stalling
};

NLOHMANN_JSON_SERIALIZE_ENUM(CvarTier, {
                                           {CvarTier::Queried, "queried"},
                                           {CvarTier::UserInfo, "userinfo"},
                                       })

enum class CvarConstraint
{
    Equals,     ///< must equal `value`
    Max,        ///< must not exceed `value`
    Range,      ///< must fall within [`value`, `max`]
    MinOrZero,  ///< must be at least `value`, or 0 for "unlimited"
    Off,        ///< must read as false or 0
    On,         ///< must not read as false or 0
};

NLOHMANN_JSON_SERIALIZE_ENUM(CvarConstraint, {
                                                 {CvarConstraint::Equals, "equals"},
                                                 {CvarConstraint::Max, "max"},
                                                 {CvarConstraint::Range, "range"},
                                                 {CvarConstraint::MinOrZero, "minOrZero"},
                                                 {CvarConstraint::Off, "off"},
                                                 {CvarConstraint::On, "on"},
                                             })

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
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CvarRule, name, tier, constraint, value, max, cheatProtected, kickOnly)

struct DetectionData
{
    std::vector<std::string> dllEventBlacklist;
    std::vector<CvarRule> cvarRules;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DetectionData, dllEventBlacklist, cvarRules)

}  // namespace Anticheat
