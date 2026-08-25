#include "Fun/FunToggles.hpp"

#include <doctest/doctest.h>

using AdminSystem::Fun::DecideDamage;
using AdminSystem::Fun::HitGroupHead;
using AdminSystem::Fun::OneHitKillDamage;
using AdminSystem::Fun::ParseToggle;
using AdminSystem::Fun::Toggle;
using AdminSystem::Fun::ToggleState;
using AdminSystem::Fun::ToggleWord;

namespace
{

constexpr int HitGroupChest = 2;
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
    auto out = DecideDamage({}, HitGroupChest, false, true, Incoming);
    CHECK_FALSE(out.Suppress);
    CHECK_EQ(out.Damage, doctest::Approx(Incoming));
}

TEST_CASE("Headshot only suppresses a body shot and allows a head shot")
{
    auto state = With(Toggle::HeadshotOnly);
    CHECK(DecideDamage(state, HitGroupChest, false, true, Incoming).Suppress);
    CHECK_FALSE(DecideDamage(state, HitGroupHead, false, true, Incoming).Suppress);
}

TEST_CASE("No-scope only suppresses a scoped attacker and allows an unscoped one")
{
    auto state = With(Toggle::NoScopeOnly);
    CHECK(DecideDamage(state, HitGroupChest, /*scoped*/ true, true, Incoming).Suppress);
    CHECK_FALSE(DecideDamage(state, HitGroupChest, /*scoped*/ false, true, Incoming).Suppress);
}

TEST_CASE("One-hit kill raises the damage past any survivable health")
{
    auto out = DecideDamage(With(Toggle::OneHitKill), HitGroupChest, false, true, Incoming);
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

    auto body = DecideDamage(state, HitGroupChest, false, true, Incoming);
    CHECK(body.Suppress);
    CHECK_EQ(body.Damage, doctest::Approx(Incoming));

    auto head = DecideDamage(state, HitGroupHead, false, true, Incoming);
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

    auto out = DecideDamage(state, HitGroupChest, /*scoped*/ true, /*hasAttacker*/ false, Incoming);
    CHECK_FALSE(out.Suppress);
    CHECK_EQ(out.Damage, doctest::Approx(Incoming));
}

TEST_CASE("Non-damage toggles do not touch the damage")
{
    for (auto toggle : {Toggle::LowGravity, Toggle::KnifeRound, Toggle::InfiniteMoney, Toggle::ChickenBots})
    {
        auto out = DecideDamage(With(toggle), HitGroupChest, false, true, Incoming);
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

TEST_CASE("Every toggle round-trips through its command word")
{
    for (std::size_t i = 0; i < AdminSystem::Fun::ToggleCount; ++i)
    {
        auto toggle = static_cast<Toggle>(i);
        auto word = ToggleWord(toggle);
        CHECK_FALSE(word.empty());
        CHECK(ParseToggle(word) == toggle);
    }
}

TEST_CASE("ParseToggle ignores case and rejects an unknown word")
{
    CHECK(ParseToggle("LowGravity") == Toggle::LowGravity);
    CHECK(ParseToggle("LOWGRAVITY") == Toggle::LowGravity);
    CHECK(ParseToggle("rocketmode") == Toggle::Count);
    CHECK(ParseToggle("") == Toggle::Count);
}
