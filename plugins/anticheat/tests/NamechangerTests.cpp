#include "Detect/Rules/Namechanger.hpp"

#include <doctest/doctest.h>
#include <format>

using Anticheat::DefaultTuning;
using Anticheat::DetectionKind;
using Anticheat::Finding;
using Anticheat::MaxSlots;
using Anticheat::Rules::Namechanger;
using Anticheat::Suspicion;

static constexpr int Slot = 3;
static constexpr double Now = 100.0;

/** The rule and the score it feeds, since a finding now comes out of the score. */
struct Harness
{
    Harness() { Scores.Configure(DefaultTuning()); }

    Suspicion Scores;
    Namechanger Rule{Scores};
};

TEST_CASE("The fifth name change inside one minute fires and the fourth does not")
{
    Harness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "original", "");

    for (int i = 1; i <= 4; ++i)
        CHECK_FALSE(rule.OnIdentity(Slot, std::format("name{}", i), "", Now).has_value());
    CHECK(rule.RecentChanges(Slot, Now) == 4);

    const std::optional<Finding> finding = rule.OnIdentity(Slot, "name5", "", Now);
    REQUIRE(finding.has_value());
    CHECK(finding->Kind == DetectionKind::Namechanger);
    CHECK_FALSE(finding->KickOnly);
    // Reporting no longer wipes the rule's own window, and never wipes the score behind it.
    CHECK(rule.RecentChanges(Slot, Now) == 5);
}

TEST_CASE("A settings change that leaves the name alone is not a change")
{
    Harness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "steady", "");
    for (int i = 0; i < 10; ++i)
        CHECK_FALSE(rule.OnIdentity(Slot, "steady", "", Now).has_value());
    CHECK(rule.RecentChanges(Slot, Now) == 0);
}

TEST_CASE("Changes older than a minute fall out of the rolling window")
{
    Harness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "original", "");
    for (int i = 1; i <= 4; ++i)
        rule.OnIdentity(Slot, std::format("name{}", i), "", Now);
    CHECK(rule.RecentChanges(Slot, Now) == 4);

    // A minute later the earlier four are gone, so this one stands alone.
    CHECK_FALSE(rule.OnIdentity(Slot, "name5", "", Now + 61.0).has_value());
    CHECK(rule.RecentChanges(Slot, Now + 61.0) == 1);
}

TEST_CASE("A change arriving before the baseline establishes it instead of counting")
{
    Harness h;
    Namechanger& rule = h.Rule;
    CHECK_FALSE(rule.OnIdentity(Slot, "first", "", Now).has_value());
    CHECK(rule.RecentChanges(Slot, Now) == 0);
    CHECK_FALSE(rule.OnIdentity(Slot, "first", "", Now).has_value());
    CHECK(rule.RecentChanges(Slot, Now) == 0);
    rule.OnIdentity(Slot, "second", "", Now);
    CHECK(rule.RecentChanges(Slot, Now) == 1);
}

TEST_CASE("An empty name is ignored rather than counted as a change")
{
    Harness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "original", "");
    CHECK_FALSE(rule.OnIdentity(Slot, "", "", Now).has_value());
    CHECK(rule.RecentChanges(Slot, Now) == 0);
}

TEST_CASE("A new baseline drops the slot's previous change history")
{
    Harness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "original", "");
    for (int i = 1; i <= 4; ++i)
        rule.OnIdentity(Slot, std::format("name{}", i), "", Now);
    rule.OnBaseline(Slot, "rejoined", "");
    CHECK(rule.RecentChanges(Slot, Now) == 0);
    CHECK_FALSE(rule.OnIdentity(Slot, "rejoined", "", Now).has_value());
}

TEST_CASE("A slot change and a reset both clear the history")
{
    Harness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "original", "");
    for (int i = 1; i <= 4; ++i)
        rule.OnIdentity(Slot, std::format("name{}", i), "", Now);

    rule.OnSlotChanged(Slot);
    CHECK(rule.RecentChanges(Slot, Now) == 0);

    rule.OnBaseline(Slot, "original", "");
    for (int i = 1; i <= 4; ++i)
        rule.OnIdentity(Slot, std::format("name{}", i), "", Now);
    rule.Reset();
    CHECK(rule.RecentChanges(Slot, Now) == 0);
}

TEST_CASE("Out of range slots are ignored rather than written past the array")
{
    Harness h;
    Namechanger& rule = h.Rule;
    CHECK_FALSE(rule.OnIdentity(-1, "name", "", Now).has_value());
    CHECK_FALSE(rule.OnIdentity(MaxSlots, "name", "", Now).has_value());
    CHECK(rule.RecentChanges(-1, Now) == 0);
}

TEST_CASE("Clan tag changes count exactly like name changes")
{
    Harness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "steady", "");

    for (int i = 1; i <= 4; ++i)
        CHECK_FALSE(rule.OnIdentity(Slot, "steady", std::format("tag{}", i), Now).has_value());
    CHECK(rule.RecentChanges(Slot, Now) == 4);

    const std::optional<Finding> finding = rule.OnIdentity(Slot, "steady", "tag5", Now);
    REQUIRE(finding.has_value());
    CHECK(finding->Kind == DetectionKind::Namechanger);
}

TEST_CASE("A repeated read of the same name and tag never counts")
{
    Harness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "steady", "[tag]");
    for (int i = 0; i < 100; ++i)
        CHECK_FALSE(rule.OnIdentity(Slot, "steady", "[tag]", Now + i * 0.125).has_value());
    CHECK(rule.RecentChanges(Slot, Now) == 0);
}

TEST_CASE("After a burst the rule waits out its cooldown, then a fresh burst counts again")
{
    Harness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "steady", "");
    for (int i = 1; i <= 5; ++i)
        rule.OnIdentity(Slot, "steady", std::format("tag{}", i), Now);
    CHECK(h.Scores.Value(Slot, DetectionKind::Namechanger, Now) == doctest::Approx(1.0f));

    // An animated tag is one offence, not one a second: nothing lands while the cooldown holds.
    for (int i = 6; i <= 40; ++i)
        CHECK_FALSE(rule.OnIdentity(Slot, "steady", std::format("tag{}", i), Now + i).has_value());
    CHECK(h.Scores.Value(Slot, DetectionKind::Namechanger, Now) == doctest::Approx(1.0f));

    // Past the cooldown a fresh burst of five counts again, on top of what has not decayed away.
    const double later = Now + 102.0;
    for (int i = 41; i <= 45; ++i)
        rule.OnIdentity(Slot, "steady", std::format("tag{}", i), later);
    CHECK(h.Scores.Value(Slot, DetectionKind::Namechanger, later) > 1.5f);
}
