#include "Fun/FunToggles.hpp"

#include <doctest/doctest.h>

using AdminSystem::Fun::Toggle;
using AdminSystem::Fun::Toggles;
using AdminSystem::Fun::ToggleState;

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
    CHECK(state.IsOn(Toggle::OneHitKill));
    CHECK_FALSE(state.IsOn(Toggle::HeadshotOnly));
    state.Clear();
    CHECK_FALSE(state.AnyOn());
}

TEST_CASE("The toggle table is indexed by the enum and fully populated")
{
    // The menu indexes by Toggle, so a misordered entry renders one modifier's label against
    // another's broadcast lines.
    for (std::size_t i = 0; i < AdminSystem::Fun::ToggleCount; ++i)
    {
        const auto& info = Toggles[i];
        CHECK(info.Id == static_cast<Toggle>(i));
        CHECK_FALSE(info.NameKey.empty());
        CHECK_FALSE(info.OnKey.empty());
        CHECK_FALSE(info.OffKey.empty());
    }
}
