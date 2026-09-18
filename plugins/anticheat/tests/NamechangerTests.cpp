#include "Detect/Rules/Namechanger.hpp"
#include "Harness.hpp"

#include <doctest/doctest.h>
#include <format>

using Anticheat::DetectionKind;
using Anticheat::Finding;
using Anticheat::MaxSlots;
using Anticheat::Suspicion;
using Anticheat::Rules::Namechanger;

static constexpr int Slot = 3;
static constexpr double Now = 100.0;

/** The rule and the score it feeds, since a finding now comes out of the score. */
struct NamechangerHarness
{
    NamechangerHarness() { Scores.ReportTo(Reported.Sink()); }

    Anticheat::Test::Findings Reported;
    Suspicion Scores;
    Namechanger Rule{Scores};
};

TEST_CASE("The fifth name change inside one minute fires and the fourth does not")
{
    NamechangerHarness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "original", "");

    for (int i = 1; i <= 4; ++i)
        rule.OnIdentity(Slot, std::format("name{}", i), "", Now);
    CHECK(rule.RecentChanges(Slot, Now) == 4);

    rule.OnIdentity(Slot, "name5", "", Now);
    REQUIRE(h.Reported.Last.has_value());
    CHECK(h.Reported.Last->Kind == DetectionKind::Namechanger);
    CHECK_FALSE(h.Reported.Last->KickOnly);
    // Reporting no longer wipes the rule's own window, and never wipes the score behind it.
    CHECK(rule.RecentChanges(Slot, Now) == 5);
}

TEST_CASE("A settings change that leaves the name alone is not a change")
{
    NamechangerHarness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "steady", "");
    for (int i = 0; i < 10; ++i)
        rule.OnIdentity(Slot, "steady", "", Now);
    CHECK(rule.RecentChanges(Slot, Now) == 0);
}

TEST_CASE("Changes older than a minute fall out of the rolling window")
{
    NamechangerHarness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "original", "");
    for (int i = 1; i <= 4; ++i)
        rule.OnIdentity(Slot, std::format("name{}", i), "", Now);
    CHECK(rule.RecentChanges(Slot, Now) == 4);

    // A minute later the earlier four are gone, so this one stands alone.
    rule.OnIdentity(Slot, "name5", "", Now + 61.0);
    CHECK(rule.RecentChanges(Slot, Now + 61.0) == 1);
}

TEST_CASE("A change arriving before the baseline establishes it instead of counting")
{
    NamechangerHarness h;
    Namechanger& rule = h.Rule;
    rule.OnIdentity(Slot, "first", "", Now);
    CHECK(rule.RecentChanges(Slot, Now) == 0);
    rule.OnIdentity(Slot, "first", "", Now);
    CHECK(rule.RecentChanges(Slot, Now) == 0);
    rule.OnIdentity(Slot, "second", "", Now);
    CHECK(rule.RecentChanges(Slot, Now) == 1);
}

TEST_CASE("An empty name is ignored rather than counted as a change")
{
    NamechangerHarness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "original", "");
    rule.OnIdentity(Slot, "", "", Now);
    CHECK(rule.RecentChanges(Slot, Now) == 0);
}

TEST_CASE("A new baseline drops the slot's previous change history")
{
    NamechangerHarness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "original", "");
    for (int i = 1; i <= 4; ++i)
        rule.OnIdentity(Slot, std::format("name{}", i), "", Now);
    rule.OnBaseline(Slot, "rejoined", "");
    CHECK(rule.RecentChanges(Slot, Now) == 0);
    rule.OnIdentity(Slot, "rejoined", "", Now);
}

TEST_CASE("A slot change and a reset both clear the history")
{
    NamechangerHarness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "original", "");
    for (int i = 1; i <= 4; ++i)
        rule.OnIdentity(Slot, std::format("name{}", i), "", Now);

    rule.ClearSlot(Slot);
    CHECK(rule.RecentChanges(Slot, Now) == 0);

    rule.OnBaseline(Slot, "original", "");
    for (int i = 1; i <= 4; ++i)
        rule.OnIdentity(Slot, std::format("name{}", i), "", Now);
    rule.Reset();
    CHECK(rule.RecentChanges(Slot, Now) == 0);
}

TEST_CASE("Out of range slots are ignored rather than written past the array")
{
    NamechangerHarness h;
    Namechanger& rule = h.Rule;
    rule.OnIdentity(-1, "name", "", Now);
    rule.OnIdentity(MaxSlots, "name", "", Now);
    CHECK(rule.RecentChanges(-1, Now) == 0);
}

TEST_CASE("Clan tag changes count exactly like name changes")
{
    NamechangerHarness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "steady", "");

    for (int i = 1; i <= 4; ++i)
        rule.OnIdentity(Slot, "steady", std::format("tag{}", i), Now);
    CHECK(rule.RecentChanges(Slot, Now) == 4);

    rule.OnIdentity(Slot, "steady", "tag5", Now);
    REQUIRE(h.Reported.Last.has_value());
    CHECK(h.Reported.Last->Kind == DetectionKind::Namechanger);
}

TEST_CASE("A repeated read of the same name and tag never counts")
{
    NamechangerHarness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "steady", "[tag]");
    for (int i = 0; i < 100; ++i)
        rule.OnIdentity(Slot, "steady", "[tag]", Now + i * 0.125);
    CHECK(rule.RecentChanges(Slot, Now) == 0);
}

TEST_CASE("After a burst the rule waits out its cooldown, then a fresh burst counts again")
{
    NamechangerHarness h;
    Namechanger& rule = h.Rule;
    rule.OnBaseline(Slot, "steady", "");
    for (int i = 1; i <= 5; ++i)
        rule.OnIdentity(Slot, "steady", std::format("tag{}", i), Now);
    CHECK(h.Scores.Value(Slot, DetectionKind::Namechanger, Now) == doctest::Approx(1.0f));

    // An animated tag is one offence, not one a second: nothing lands while the cooldown holds.
    for (int i = 6; i <= 40; ++i)
        rule.OnIdentity(Slot, "steady", std::format("tag{}", i), Now + i);
    CHECK(h.Scores.Value(Slot, DetectionKind::Namechanger, Now) == doctest::Approx(1.0f));

    // Past the cooldown a fresh burst of five counts again, on top of what has not decayed away.
    const double later = Now + 102.0;
    for (int i = 41; i <= 45; ++i)
        rule.OnIdentity(Slot, "steady", std::format("tag{}", i), later);
    CHECK(h.Scores.Value(Slot, DetectionKind::Namechanger, later) > 1.5f);
}
