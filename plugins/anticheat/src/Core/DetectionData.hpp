#pragma once

#include <VoltMod/Core/Json.hpp>
#include <VoltMod/Core/Result.hpp>

// Reloadable Valve event and convar data. Parsing rejects unknown tokens and
// missing keys so a typo cannot silently change a detector rule. Kept SDK-free
// for unit tests.

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Anticheat
{

inline constexpr std::string_view DetectionDataPath = "addons/anticheat/configs/detections.jsonc";

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

/** The event/convar tables the detections compare against. */
struct DetectionData
{
    std::vector<std::string> dllEventBlacklist;
    std::vector<CvarRule> cvarRules;
};

/** The file shape before conditional validation. */
struct DetectionDocument
{
    struct Rule
    {
        std::string name;
        CvarTier tier = CvarTier::Queried;
        std::optional<CvarConstraint> constraint;
        std::optional<double> value;
        std::optional<double> max;
        bool cheatProtected = false;
        bool kickOnly = false;
    };

    std::optional<std::vector<std::string>> dllEventBlacklist;
    std::optional<std::vector<Rule>> cvarRules;
};

/**
 * @brief Apply every rule the document's shape cannot state.
 *
 * Both sections must be present (a renamed section must fail rather than silently disabling its
 * detector), a rule needs a non-empty name and a constraint, a numeric constraint needs its
 * `value`, and `range` needs a `max` that is not below it.
 */
VoltMod::Result<DetectionData> ValidateDetectionData(DetectionDocument document);

/** Loads @ref DetectionData, validating it before publishing. */
class DetectionDataManager
{
public:
    /** On failure the previously loaded tables stand unchanged. */
    VoltMod::Status Load(std::string_view path);

    const DetectionData& Get() const { return _data; }

private:
    DetectionData _data;
};

}  // namespace Anticheat

/** The tokens the file uses; an unrecognized one is rejected by name. */
template <>
struct glz::meta<Anticheat::CvarTier>
{
    using enum Anticheat::CvarTier;
    static constexpr auto value = glz::enumerate("queried", Queried, "userinfo", UserInfo);
};

template <>
struct glz::meta<Anticheat::CvarConstraint>
{
    using enum Anticheat::CvarConstraint;
    static constexpr auto value =
        glz::enumerate("equals", Equals, "max", Max, "range", Range, "minOrZero", MinOrZero, "off", Off, "on", On);
};
