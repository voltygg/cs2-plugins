#pragma once

#include "FunToggles.hpp"

#include <VoltMod/Core/Subscription.hpp>
#include <string>
#include <vector>

namespace VoltMod
{
class Runtime;
}

namespace AdminSystem
{
struct App;
}

namespace AdminSystem::Fun
{

/**
 * Server-wide round modifiers: low gravity, headshot-only, knife rounds and the rest.
 *
 * App-owned. Everything here is round-scoped by intent - the state survives a round so an admin
 * does not have to re-enable it every round, but the effects it applies (gravity, weapons,
 * models) are re-applied at each round start and undone when the last toggle goes off.
 */
class FunMode
{
public:
    explicit FunMode(App& app);

    /** Subscribe to the round and spawn events the toggles need. Call once during load. */
    void Start();

    /** Flip @p toggle and apply or undo its effect immediately. @return the new state. */
    bool Flip(Toggle toggle);

    bool IsOn(Toggle toggle) const { return _state.IsOn(toggle); }
    const ToggleState& State() const { return _state; }

    /** Turn everything off and restore what the toggles changed. */
    void ClearAll();

private:
    void ApplyRoundStart();
    void ApplyGravity();
    void GiveKnifeOnly(int slot);
    void MakeChicken(int slot);
    void TopUpMoney();
    void OnDamage(VoltMod::DamageView& view);
    /** Whether @p slot is currently scoped in (CCSPlayerPawn::m_bIsScoped). */
    bool IsScoped(int slot) const;

    App& _app;
    ToggleState _state;
    /** Listener registrations, released together and declared last so they stop before the
     *  state their callbacks read. */
    std::vector<VoltMod::Subscription> _subs;
};

}  // namespace AdminSystem::Fun
