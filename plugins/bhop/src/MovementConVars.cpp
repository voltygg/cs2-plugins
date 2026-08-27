#include "MovementConVars.hpp"

#include <VoltMod/Core/Log.hpp>
#include <string>
#include <utility>

namespace Log = VoltMod::Log;

namespace Bhop
{

template <class T>
void MovementConVars::Add(const char* name, T value, std::vector<Override<T>>& into)
{
    auto cvar = VoltMod::ConVar<T>::Find(_conVars, name);
    if (!cvar)
    {
        Log::Warn("ConVar '{}' unusable ({}); bhop override skipped.", name, cvar.error().Detail);
        return;
    }

    into.push_back({.Cvar = std::move(*cvar), .Value = value});
}

void MovementConVars::Build(const BhopSettings& settings)
{
    _flags.clear();
    _numbers.clear();

    if (settings.autoBunnyhopping)
        Add("sv_autobunnyhopping", true, _flags);
    if (settings.enableBunnyhopping)
        Add("sv_enablebunnyhopping", true, _flags);
    if (settings.staminaJumpCost >= 0.0f)
        Add("sv_staminajumpcost", settings.staminaJumpCost, _numbers);
    if (settings.staminaLandCost >= 0.0f)
        Add("sv_staminalandcost", settings.staminaLandCost, _numbers);
    if (settings.airAccelerate >= 0.0f)
        Add("sv_airaccelerate", settings.airAccelerate, _numbers);
    if (settings.airMaxWishSpeed >= 0.0f)
        Add("sv_air_max_wishspeed", settings.airMaxWishSpeed, _numbers);
    if (settings.maxVelocity >= 0.0f)
        Add("sv_maxvelocity", settings.maxVelocity, _numbers);
}

void MovementConVars::Reset()
{
    ReleaseRaw();
    RestoreGlobal();
    _flags.clear();
    _numbers.clear();
}

void MovementConVars::ApplyGlobal()
{
    // Override() saves on the first take and re-asserts afterwards, which is what lets this be
    // called again after a map change without saving the override as the operator's own value.
    // It writes through the console: a server-only set leaves FCVAR_REPLICATED movement convars
    // unnetworked, and clients keep predicting the defaults - no auto-hop, no speed retention.
    for (auto& entry : _flags)
        _globalOverrides.Set(entry.Cvar, entry.Value);
    for (auto& entry : _numbers)
        _globalOverrides.Set(entry.Cvar, entry.Value);
}

void MovementConVars::RestoreGlobal()
{
    _globalOverrides.RestoreAll();
}

void MovementConVars::ReplicateOverrides(int slot)
{
    for (auto& entry : _flags)
        (void)entry.Cvar.SetFor(slot, entry.Value);
    for (auto& entry : _numbers)
        (void)entry.Cvar.SetFor(slot, entry.Value);
}

void MovementConVars::ReplicateServerValues(int slot)
{
    for (auto& entry : _flags)
        (void)entry.Cvar.SetFor(slot, entry.Cvar.Get());
    for (auto& entry : _numbers)
        (void)entry.Cvar.SetFor(slot, entry.Cvar.Get());
}

void MovementConVars::HoldRaw()
{
    // Raw flips: server-side movement for this player's command sees bhop values; no callbacks
    // fire and nothing is networked. The scopes put the real values back in ReleaseRaw, before
    // anyone else runs.
    for (auto& entry : _flags)
        _flagFlips.push_back(entry.Cvar.RawScope(entry.Value));
    for (auto& entry : _numbers)
        _numberFlips.push_back(entry.Cvar.RawScope(entry.Value));
}

void MovementConVars::ReleaseRaw()
{
    _numberFlips.clear();
    _flagFlips.clear();
}

}  // namespace Bhop
