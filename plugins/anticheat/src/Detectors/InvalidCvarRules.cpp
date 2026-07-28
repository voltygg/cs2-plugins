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
constexpr double MaximumMouseYaw = 0.3;
constexpr double MinimumFpsMax = 64.0;
constexpr double MinimumSensitivity = 0.0001;
// The engine caps sensitivity at 8; the ceiling is deliberately loose so a future engine bump
// cannot turn a legitimate setting into a detection.
constexpr double MaximumSensitivity = 20.0;
constexpr double RequiredPitchLimit = 89.0;
constexpr double RequiredYawSpeed = 210.0;

bool EqualsNoCase(std::string_view a, std::string_view b)
{
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
           });
}

/** Whole-string numeric parse; trailing junk makes the value invalid, not merely odd. */
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

CvarVerdict Invalid(std::string reason, bool kickOnly = false)
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

std::string NotANumber(std::string_view cvar)
{
    return std::format("{} did not return a valid finite number.", cvar);
}

/** Rules that only mean something once the server's disabled sv_cheats has reached the client. */
constexpr std::string_view CheatProtectedCvars[] = {"sv_cheats", "cl_showpos", "cam_showangles", "cl_drawhud",
                                                    "fov_cs_debug"};

bool IsCheatProtected(std::string_view name)
{
    return std::ranges::any_of(CheatProtectedCvars, [&](std::string_view cvar) { return EqualsNoCase(cvar, name); });
}

/** Position of @p name in @ref RuledCvars, or -1 when the rule table does not cover it. */
int CvarIndex(std::string_view name)
{
    for (size_t index = 0; index < std::size(RuledCvars); ++index)
        if (EqualsNoCase(RuledCvars[index], name))
            return static_cast<int>(index);
    return -1;
}
}  // namespace

bool ShouldEnforceCheatCvars(bool svCheatsEnabled, double nowSec, double graceUntilSec)
{
    return !svCheatsEnabled && nowSec > graceUntilSec;
}

CvarVerdict EvaluateCvar(std::string_view name, std::string_view value, bool enforceCheatCvars)
{
    double number = 0.0;
    const bool numeric = ParseNumber(value, number);

    if (EqualsNoCase(name, "m_yaw"))
    {
        if (!numeric)
            return Invalid(NotANumber("m_yaw"));
        if (number > MaximumMouseYaw)
            return Invalid(std::format("m_yaw is {:.6g}, but the maximum allowed value is 0.3.", number), true);
        return Valid();
    }
    if (EqualsNoCase(name, "fps_max"))
    {
        if (!numeric)
            return Invalid(NotANumber("fps_max"));
        if (number > 0.0 && number < MinimumFpsMax)
            return Invalid(std::format("fps_max is {:.6g}, but it must be at least 64 or 0 for unlimited.", number),
                           true);
        return Valid();
    }
    if (EqualsNoCase(name, "sensitivity"))
    {
        if (!numeric)
            return Invalid(NotANumber("sensitivity"));
        if (number < MinimumSensitivity || number > MaximumSensitivity)
            return Invalid(std::format("sensitivity is {:.6g}, but it must be between 0.0001 and 20.", number));
        return Valid();
    }
    if (EqualsNoCase(name, "cl_pitchdown") || EqualsNoCase(name, "cl_pitchup"))
    {
        if (!numeric)
            return Invalid(NotANumber(name));
        if (number != RequiredPitchLimit)
            return Invalid(std::format("{} is {:.6g}, but it must be 89.", name, number));
        return Valid();
    }
    if (EqualsNoCase(name, "cl_yawspeed"))
    {
        if (!numeric)
            return Invalid(NotANumber("cl_yawspeed"));
        if (number != RequiredYawSpeed)
            return Invalid(std::format("cl_yawspeed is {:.6g}, but it must be 210.", number));
        return Valid();
    }

    // Everything below is protected by sv_cheats, so it only means something once the server's
    // disabled value has certainly reached the client.
    if (IsCheatProtected(name) && !enforceCheatCvars)
        return Skipped();

    if (EqualsNoCase(name, "sv_cheats"))
    {
        if (!IsOff(value, numeric, number))
            return Invalid("sv_cheats is enabled or invalid on the client while it is disabled on the server.");
        return Valid();
    }
    if (EqualsNoCase(name, "cl_showpos") || EqualsNoCase(name, "cam_showangles"))
    {
        if (!IsOff(value, numeric, number))
            return Invalid(
                std::format("{} is enabled on the client despite sv_cheats being disabled on the server.", name));
        return Valid();
    }
    if (EqualsNoCase(name, "cl_drawhud"))
    {
        if (IsOff(value, numeric, number))
            return Invalid("cl_drawhud is disabled on the client despite sv_cheats being disabled on the server.");
        return Valid();
    }
    if (EqualsNoCase(name, "fov_cs_debug"))
    {
        if (!numeric)
            return Invalid(NotANumber("fov_cs_debug"));
        if (number != 0.0)
            return Invalid(std::format("fov_cs_debug is {:.6g}, but it must be 0.", number));
        return Valid();
    }

    return {};
}

CvarVerdict EvaluateMissingCvar(std::string_view name, std::string_view statusName, bool enforceCheatCvars,
                                int consecutiveReplies)
{
    if (CvarIndex(name) < 0)
        return {};
    if (!IsCheatProtected(name) || !enforceCheatCvars || consecutiveReplies < MissingRepliesBeforeEvidence)
        return Skipped();
    return Invalid(std::format("{} could not be read from the client on {} replies in a row (latest status {}), "
                               "although every stock client reports it.",
                               name, consecutiveReplies, statusName),
                   true);
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

std::optional<Finding> InvalidCvarRules::Observe(int slot, std::string_view name, std::string_view value,
                                                 bool enforceCheatCvars)
{
    // A reply that carries a value - whatever the value is - ends any run of refusals.
    if (const int index = CvarIndex(name); InSlotRange(slot) && index >= 0)
        _missingReplies[slot][static_cast<size_t>(index)] = 0;
    return Apply(slot, name, EvaluateCvar(name, value, enforceCheatCvars));
}

std::optional<Finding> InvalidCvarRules::ObserveMissing(int slot, std::string_view name, std::string_view statusName,
                                                        bool enforceCheatCvars)
{
    const int index = CvarIndex(name);
    if (!InSlotRange(slot) || index < 0)
        return {};

    int& consecutive = _missingReplies[slot][static_cast<size_t>(index)];
    if (consecutive < MissingRepliesBeforeEvidence)
        ++consecutive;
    return Apply(slot, name, EvaluateMissingCvar(name, statusName, enforceCheatCvars, consecutive));
}

std::optional<Finding> InvalidCvarRules::Apply(int slot, std::string_view name, const CvarVerdict& verdict)
{
    std::optional<Finding> out;
    const int index = CvarIndex(name);
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
    const int index = CvarIndex(name);
    return InSlotRange(slot) && index >= 0 && _latched[slot][static_cast<size_t>(index)];
}

}  // namespace Anticheat
