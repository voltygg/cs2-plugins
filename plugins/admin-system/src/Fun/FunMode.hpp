#pragma once

#include "FunToggles.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Subscription.hpp>
#include <array>
#include <string_view>
#include <vector>

namespace AdminSystem::Fun
{

/** One convar a toggle takes over, and the value it holds while the toggle is on. What it goes
 *  back to is the operator's own value, saved by ConVarLease before the first write. */
struct ToggleConVar
{
    Toggle Owner;
    std::string_view Name;
    float OnValue;
};

/**
 * A resolved @ref ToggleConVar.
 *
 * The rows do not share one engine type - the damage scales and gravity are floats while
 * headshot-only is a bool - so each resolves as whichever it is and the other handle stays empty.
 * A row the server does not have leaves both empty and is skipped.
 */
struct ToggleHandle
{
    Toggle Owner;
    float OnValue = 0.0f;
    VoltMod::ConVar<float> Number;
    VoltMod::ConVar<bool> Flag;
};

/**
 * Every convar the toggles drive, in no particular order.
 *
 * One-hit kill takes all four `mp_damage_scale_*` so it is symmetric across teams and still lands
 * once headshot-only has narrowed the hits down to heads. The lethal scale turns the weakest hit
 * into a few hundred damage, which kills through armor and hitgroup scaling - measured in game, a
 * 23-damage leg hit lands as ~490.
 */
inline constexpr std::array<ToggleConVar, 6> ToggleConVars{{
    {Toggle::LowGravity, "sv_gravity", 250.0f},
    {Toggle::HeadshotOnly, "mp_damage_headshot_only", 1.0f},
    {Toggle::OneHitKill, "mp_damage_scale_ct_body", 20.0f},
    {Toggle::OneHitKill, "mp_damage_scale_t_body", 20.0f},
    {Toggle::OneHitKill, "mp_damage_scale_ct_head", 20.0f},
    {Toggle::OneHitKill, "mp_damage_scale_t_head", 20.0f},
}};

/**
 * Server-wide round modifiers: low gravity, headshot-only, knife rounds and the rest.
 *
 * App-owned. Everything here is round-scoped by intent - the state survives a round so an admin
 * does not have to re-enable it every round, but the effects it applies (gravity, damage rules,
 * weapons) are re-applied at each round start and undone when the toggle goes off.
 *
 * The damage-affecting toggles drive the engine's own convars rather than the damage hook, which
 * only observes damage (see VoltMod::Damage). ConVarLease holds the saved values, so a
 * server that never turned a toggle on keeps its own cfg.
 */
class FunMode
{
public:
    /** @p runtime must outlive this object; App declares it above. */
    explicit FunMode(VoltMod::Runtime& runtime);
    ~FunMode();
    FunMode(const FunMode&) = delete;
    FunMode& operator=(const FunMode&) = delete;

    /** Subscribe to the round and spawn events the toggles need. Call once during load. */
    void Start();

    /** Flip @p toggle and apply or undo its effect immediately. @return the new state. */
    bool Flip(Toggle toggle);

    bool IsOn(Toggle toggle) const { return _state.IsOn(toggle); }

    /** Turn everything off and restore what the toggles changed. */
    void ClearAll();

private:
    void ApplyRoundStart();
    /** Resolve every @ref ToggleConVars row into a typed handle. Runs once, from Start. */
    void ResolveConVars();
    /** Take over or hand back every @ref ToggleConVars row to match the current toggle state. */
    void ApplyOverrides();
    void GiveKnifeOnly(int slot);

    VoltMod::Runtime& _rt;
    ToggleState _state;
    /** @ref ToggleConVars resolved once in Start; empty until then. */
    std::vector<ToggleHandle> _handles;
    /** Restores whatever the toggles took over, including on unload. */
    VoltMod::ConVarLease _lease;
    /** Listener registrations, released together and declared last so they stop before the
     *  state their callbacks read. */
    std::vector<VoltMod::Subscription> _subs;
};

}  // namespace AdminSystem::Fun
