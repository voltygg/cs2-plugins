#include "FunToggles.hpp"

#include <algorithm>

namespace AdminSystem::Fun
{

bool ToggleState::AnyOn() const
{
    return std::ranges::any_of(Active, [](bool on) { return on; });
}

DamageDecision DecideDamage(const ToggleState& state, VoltMod::Sdk::HitGroup hitGroup, bool attackerScoped,
                            bool hasAttacker, float incoming)
{
    DamageDecision decision{.Suppress = false, .Damage = incoming};

    // World damage (fall, fire, the bomb) has no aim to judge, so the aim rules leave it alone -
    // otherwise a headshot-only round would make players immortal to everything but bullets.
    if (hasAttacker)
    {
        if (state.IsOn(Toggle::HeadshotOnly) && hitGroup != VoltMod::Sdk::HitGroup::Head)
            decision.Suppress = true;
        if (state.IsOn(Toggle::NoScopeOnly) && attackerScoped)
            decision.Suppress = true;
    }

    // A suppressed hit is not allowed to land, so there is nothing to amplify.
    if (!decision.Suppress && state.IsOn(Toggle::OneHitKill))
        decision.Damage = OneHitKillDamage;

    return decision;
}

}  // namespace AdminSystem::Fun
