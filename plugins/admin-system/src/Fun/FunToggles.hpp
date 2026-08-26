#pragma once

#include <VoltMod/Sdk/Entity/HitGroup.hpp>
#include <array>
#include <cstdint>
#include <string_view>

namespace AdminSystem::Fun
{

/** The server-wide round modifiers, in the order the Fun Mode menu lists them. */
enum class Toggle : uint8_t
{
    LowGravity,
    HeadshotOnly,
    KnifeRound,
    NoScopeOnly,
    OneHitKill,
    Count,
};

inline constexpr std::size_t ToggleCount = static_cast<std::size_t>(Toggle::Count);

/** Which toggles are on. Plain state so the decision logic stays testable. */
struct ToggleState
{
    bool Active[ToggleCount]{};

    bool IsOn(Toggle toggle) const { return Active[static_cast<std::size_t>(toggle)]; }
    void Set(Toggle toggle, bool on) { Active[static_cast<std::size_t>(toggle)] = on; }
    bool Flip(Toggle toggle)
    {
        auto& slot = Active[static_cast<std::size_t>(toggle)];
        slot = !slot;
        return slot;
    }
    void Clear() { *this = {}; }
    bool AnyOn() const;
};

/** The translation keys for one toggle: its menu label and its two broadcast lines. */
struct ToggleInfo
{
    Toggle Id;
    std::string_view NameKey;
    std::string_view OnKey;
    std::string_view OffKey;
};

/**
 * Every toggle, in menu order and indexable by @ref Toggle.
 *
 * One table keyed by the enum, so a new modifier cannot be added with its label and its
 * broadcast lines out of step.
 */
inline constexpr std::array<ToggleInfo, ToggleCount> Toggles{{
    {Toggle::LowGravity, "fun.lowGravity", "broadcast.lowGravityOn", "broadcast.lowGravityOff"},
    {Toggle::HeadshotOnly, "fun.headshotOnly", "broadcast.headshotOnlyOn", "broadcast.headshotOnlyOff"},
    {Toggle::KnifeRound, "fun.knifeRound", "broadcast.knifeRoundOn", "broadcast.knifeRoundOff"},
    {Toggle::NoScopeOnly, "fun.noScopeOnly", "broadcast.noScopeOnlyOn", "broadcast.noScopeOnlyOff"},
    {Toggle::OneHitKill, "fun.oneHitKill", "broadcast.oneHitKillOn", "broadcast.oneHitKillOff"},
}};

/**
 * What the damage rules make of one hit.
 *
 * Pure so the interaction between the toggles is testable without an engine: headshot-only and
 * no-scope-only suppress, one-hit-kill amplifies, and the three have to compose sensibly when
 * more than one is on.
 */
struct DamageDecision
{
    bool Suppress = false;
    float Damage = 0.0f;
};

/**
 * Apply the active damage-affecting toggles to one hit.
 *
 * @param hitGroup the engine hitgroup that was struck.
 * @param attackerScoped whether the attacker was scoped in.
 * @param incoming the damage the engine was about to apply.
 *
 * A suppressing rule wins over one-hit-kill: it makes no sense to amplify a hit that is not
 * allowed to land at all. Damage from the world (no attacker) is never suppressed by the aim
 * rules, so fall damage and fire still work during a headshot-only round.
 */
DamageDecision DecideDamage(const ToggleState& state, VoltMod::Sdk::HitGroup hitGroup, bool attackerScoped,
                            bool hasAttacker, float incoming);

/** Damage that kills any player outright; well above the maximum health an admin can set. */
inline constexpr float OneHitKillDamage = 10000.0f;

}  // namespace AdminSystem::Fun
