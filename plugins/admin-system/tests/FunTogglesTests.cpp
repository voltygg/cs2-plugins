#include "Fun/FunToggles.hpp"

#include <doctest/doctest.h>

using AdminSystem::Fun::DecideDamage;
using AdminSystem::Fun::OneHitKillDamage;
using AdminSystem::Fun::Toggle;
using AdminSystem::Fun::Toggles;
using AdminSystem::Fun::ToggleState;
using VoltMod::Sdk::HitGroup;

namespace
{

constexpr float Incoming = 27.0f;

ToggleState With(Toggle toggle)
{
    ToggleState state;
    state.Set(toggle, true);
    return state;
}

}  // namespace

TEST_CASE("No toggles leaves the damage untouched")
{
    auto out = DecideDamage({}, HitGroup::Chest, false, true, Incoming);
    CHECK_FALSE(out.Suppress);
    CHECK_EQ(out.Damage, doctest::Approx(Incoming));
}

TEST_CASE("Headshot only suppresses a body shot and allows a head shot")
{
    auto state = With(Toggle::HeadshotOnly);
    CHECK(DecideDamage(state, HitGroup::Chest, false, true, Incoming).Suppress);
    CHECK_FALSE(DecideDamage(state, HitGroup::Head, false, true, Incoming).Suppress);
}

TEST_CASE("No-scope only suppresses a scoped attacker and allows an unscoped one")
{
    auto state = With(Toggle::NoScopeOnly);
    CHECK(DecideDamage(state, HitGroup::Chest, /*scoped*/ true, true, Incoming).Suppress);
    CHECK_FALSE(DecideDamage(state, HitGroup::Chest, /*scoped*/ false, true, Incoming).Suppress);
}

TEST_CASE("One-hit kill raises the damage past any survivable health")
{
    auto out = DecideDamage(With(Toggle::OneHitKill), HitGroup::Chest, false, true, Incoming);
    CHECK_FALSE(out.Suppress);
    CHECK_EQ(out.Damage, doctest::Approx(OneHitKillDamage));
}

TEST_CASE("A suppressing rule wins over one-hit kill")
{
    // Amplifying a hit that is not allowed to land would be incoherent, and would matter if the
    // engine ever applied the amount despite the suppression flag.
    ToggleState state;
    state.Set(Toggle::HeadshotOnly, true);
    state.Set(Toggle::OneHitKill, true);

    auto body = DecideDamage(state, HitGroup::Chest, false, true, Incoming);
    CHECK(body.Suppress);
    CHECK_EQ(body.Damage, doctest::Approx(Incoming));

    auto head = DecideDamage(state, HitGroup::Head, false, true, Incoming);
    CHECK_FALSE(head.Suppress);
    CHECK_EQ(head.Damage, doctest::Approx(OneHitKillDamage));
}

TEST_CASE("World damage escapes the aim rules")
{
    // Fall damage and fire have no aim to judge; suppressing them would make players immortal
    // to everything but bullets during a headshot-only round.
    ToggleState state;
    state.Set(Toggle::HeadshotOnly, true);
    state.Set(Toggle::NoScopeOnly, true);

    auto out = DecideDamage(state, HitGroup::Chest, /*scoped*/ true, /*hasAttacker*/ false, Incoming);
    CHECK_FALSE(out.Suppress);
    CHECK_EQ(out.Damage, doctest::Approx(Incoming));
}

TEST_CASE("Non-damage toggles do not touch the damage")
{
    for (auto toggle : {Toggle::LowGravity, Toggle::KnifeRound})
    {
        auto out = DecideDamage(With(toggle), HitGroup::Chest, false, true, Incoming);
        CHECK_FALSE(out.Suppress);
        CHECK_EQ(out.Damage, doctest::Approx(Incoming));
    }
}

TEST_CASE("ToggleState flips and clears")
{
    ToggleState state;
    CHECK_FALSE(state.AnyOn());
    CHECK(state.Flip(Toggle::LowGravity));
    CHECK(state.IsOn(Toggle::LowGravity));
    CHECK(state.AnyOn());
    CHECK_FALSE(state.Flip(Toggle::LowGravity));
    CHECK_FALSE(state.AnyOn());

    state.Set(Toggle::OneHitKill, true);
    state.Clear();
    CHECK_FALSE(state.AnyOn());
}

TEST_CASE("The toggle table is indexed by the enum and fully populated")
{
    // The menu indexes this table by Toggle, so a misordered or half-filled entry would render
    // one modifier's label against another's broadcast lines.
    for (std::size_t i = 0; i < AdminSystem::Fun::ToggleCount; ++i)
    {
        const auto& info = Toggles[i];
        CHECK(info.Id == static_cast<Toggle>(i));
        CHECK_FALSE(info.NameKey.empty());
        CHECK_FALSE(info.OnKey.empty());
        CHECK_FALSE(info.OffKey.empty());
    }
}
