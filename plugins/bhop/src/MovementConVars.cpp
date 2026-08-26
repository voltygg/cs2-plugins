#include "MovementConVars.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>

using namespace VoltMod;
namespace Log = VoltMod::Log;

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
    // Take() snapshots on the first take and re-asserts afterwards, which is what lets this be
    // called again after a map change without saving the override as the operator's own value.
    // It writes through the server-console path: the direct setters change only the server's
    // stored value, so FCVAR_REPLICATED movement convars never network to clients and their
    // predicted movement keeps the defaults - no auto-hop, no speed retention.
    for (const auto& entry : _overrides)
        _global.Take(entry.Name, entry.NetValue);
}

void MovementConVars::RestoreGlobal()
{
    _global.ReleaseAll();
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
