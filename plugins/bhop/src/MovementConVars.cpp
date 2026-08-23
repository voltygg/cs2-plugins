#include "MovementConVars.hpp"

#include <CS2Kit/Core/Log.hpp>
#include <CS2Kit/Runtime.hpp>
#include <format>

using namespace CS2Kit;
namespace Log = CS2Kit::Log;

namespace Bhop
{

namespace
{

std::string FormatConVarValue(bool isFloat, float value)
{
    return isFloat ? std::format("{}", value) : (value != 0.0f ? "1" : "0");
}

}  // namespace

void MovementConVars::Build(const BhopSettings& settings)
{
    _overrides.clear();

    auto add = [this](const char* name, bool isFloat, float value) {
        _overrides.push_back({.Name = name,
                              .IsFloat = isFloat,
                              .Value = value,
                              .NetValue = FormatConVarValue(isFloat, value),
                              .Raw = _conVars.Raw(name)});
        if (!_overrides.back().Raw.Valid())
            Log::Warn("ConVar '{}' not found; bhop override skipped in grants mode.", name);
    };

    if (settings.autoBunnyhopping)
        add("sv_autobunnyhopping", false, 1.0f);
    if (settings.enableBunnyhopping)
        add("sv_enablebunnyhopping", false, 1.0f);
    if (settings.staminaJumpCost >= 0.0f)
        add("sv_staminajumpcost", true, settings.staminaJumpCost);
    if (settings.staminaLandCost >= 0.0f)
        add("sv_staminalandcost", true, settings.staminaLandCost);
    if (settings.airAccelerate >= 0.0f)
        add("sv_airaccelerate", true, settings.airAccelerate);
    if (settings.airMaxWishSpeed >= 0.0f)
        add("sv_air_max_wishspeed", true, settings.airMaxWishSpeed);
    if (settings.maxVelocity >= 0.0f)
        add("sv_maxvelocity", true, settings.maxVelocity);
}

void MovementConVars::Reset()
{
    RestoreGlobal();
    _overrides.clear();
}

void MovementConVars::ApplyGlobal()
{
    auto& conVars = _conVars;
    for (auto& entry : _overrides)
    {
        if (!_globalApplied)
            entry.SavedValue = entry.IsFloat ? conVars.GetFloat(entry.Name).value_or(entry.Value)
                                             : (conVars.GetBool(entry.Name).value_or(false) ? 1.0f : 0.0f);

        // Set through the server-console path (as a cfg line would). The direct setters change
        // only the server's stored value, so FCVAR_REPLICATED movement convars never network to
        // clients and their predicted movement keeps the defaults - no auto-hop, no speed retention.
        // The command path both sets and replicates.
        conVars.ExecuteServerCommand(std::format("{} {}", entry.Name, entry.NetValue).c_str());
    }
    _globalApplied = true;
}

void MovementConVars::RestoreGlobal()
{
    if (!_globalApplied)
        return;

    for (const auto& entry : _overrides)
        _conVars.ExecuteServerCommand(
            std::format("{} {}", entry.Name, FormatConVarValue(entry.IsFloat, entry.SavedValue)).c_str());
    _globalApplied = false;
}

void MovementConVars::ReplicateOverrides(int slot) const
{
    for (const auto& entry : _overrides)
        _conVars.ReplicateToClient(slot, entry.Name, entry.NetValue.c_str());
}

void MovementConVars::ReplicateServerValues(int slot) const
{
    for (const auto& entry : _overrides)
    {
        auto value = _conVars.GetString(entry.Name);
        if (value)
            _conVars.ReplicateToClient(slot, entry.Name, value->c_str());
    }
}

void MovementConVars::FlipRaw()
{
    // Raw flips: server-side movement for this player's command sees bhop values; no callbacks
    // fire and nothing is networked. RestoreRaw undoes it before anyone else runs.
    for (auto& entry : _overrides)
    {
        if (!entry.Raw.Valid())
            continue;
        if (entry.IsFloat)
        {
            entry.SavedValue = entry.Raw.GetFloat();
            entry.Raw.SetFloat(entry.Value);
        }
        else
        {
            entry.SavedValue = entry.Raw.GetBool() ? 1.0f : 0.0f;
            entry.Raw.SetBool(entry.Value != 0.0f);
        }
    }
    _flipped = true;
}

void MovementConVars::RestoreRaw()
{
    if (!_flipped)
        return;

    for (auto& entry : _overrides)
    {
        if (!entry.Raw.Valid())
            continue;
        if (entry.IsFloat)
            entry.Raw.SetFloat(entry.SavedValue);
        else
            entry.Raw.SetBool(entry.SavedValue != 0.0f);
    }
    _flipped = false;
}

}  // namespace Bhop
