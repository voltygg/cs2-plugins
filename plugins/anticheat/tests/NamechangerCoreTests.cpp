#include "Detectors/NamechangerCore.hpp"

#include <doctest/doctest.h>
#include <format>

using Anticheat::DetectionKind;
using Anticheat::Finding;
using Anticheat::MaxSlots;
using Anticheat::NamechangerCore;

static constexpr int Slot = 3;
static constexpr double Now = 100.0;

TEST_CASE("The fifth name change inside one minute fires and the fourth does not")
{
    NamechangerCore core;
    core.OnBaseline(Slot, "original", "");

    for (int i = 1; i <= 4; ++i)
        CHECK_FALSE(core.OnIdentity(Slot, std::format("name{}", i), "", Now).has_value());
    CHECK(core.ChangeCount(Slot) == 4);

    const std::optional<Finding> finding = core.OnIdentity(Slot, "name5", "", Now);
    REQUIRE(finding.has_value());
    CHECK(finding->Kind == DetectionKind::Namechanger);
    CHECK_FALSE(finding->KickOnly);
    CHECK(core.ChangeCount(Slot) == 0);  // firing clears the window
}

TEST_CASE("A settings change that leaves the name alone is not a change")
{
    NamechangerCore core;
    core.OnBaseline(Slot, "steady", "");
    for (int i = 0; i < 10; ++i)
        CHECK_FALSE(core.OnIdentity(Slot, "steady", "", Now).has_value());
    CHECK(core.ChangeCount(Slot) == 0);
}

TEST_CASE("Changes older than a minute fall out of the rolling window")
{
    NamechangerCore core;
    core.OnBaseline(Slot, "original", "");
    for (int i = 1; i <= 4; ++i)
        core.OnIdentity(Slot, std::format("name{}", i), "", Now);
    CHECK(core.ChangeCount(Slot) == 4);

    // A minute later the earlier four are gone, so this one stands alone.
    CHECK_FALSE(core.OnIdentity(Slot, "name5", "", Now + 61.0).has_value());
    CHECK(core.ChangeCount(Slot) == 1);
}

TEST_CASE("A change arriving before the baseline establishes it instead of counting")
{
    NamechangerCore core;
    CHECK_FALSE(core.OnIdentity(Slot, "first", "", Now).has_value());
    CHECK(core.ChangeCount(Slot) == 0);
    CHECK_FALSE(core.OnIdentity(Slot, "first", "", Now).has_value());
    CHECK(core.ChangeCount(Slot) == 0);
    core.OnIdentity(Slot, "second", "", Now);
    CHECK(core.ChangeCount(Slot) == 1);
}

TEST_CASE("An empty name is ignored rather than counted as a change")
{
    NamechangerCore core;
    core.OnBaseline(Slot, "original", "");
    CHECK_FALSE(core.OnIdentity(Slot, "", "", Now).has_value());
    CHECK(core.ChangeCount(Slot) == 0);
}

TEST_CASE("A new baseline drops the slot's previous change history")
{
    NamechangerCore core;
    core.OnBaseline(Slot, "original", "");
    for (int i = 1; i <= 4; ++i)
        core.OnIdentity(Slot, std::format("name{}", i), "", Now);
    core.OnBaseline(Slot, "rejoined", "");
    CHECK(core.ChangeCount(Slot) == 0);
    CHECK_FALSE(core.OnIdentity(Slot, "rejoined", "", Now).has_value());
}

TEST_CASE("A slot change and a reset both clear the history")
{
    NamechangerCore core;
    core.OnBaseline(Slot, "original", "");
    for (int i = 1; i <= 4; ++i)
        core.OnIdentity(Slot, std::format("name{}", i), "", Now);

    core.OnSlotChanged(Slot);
    CHECK(core.ChangeCount(Slot) == 0);

    core.OnBaseline(Slot, "original", "");
    for (int i = 1; i <= 4; ++i)
        core.OnIdentity(Slot, std::format("name{}", i), "", Now);
    core.Reset();
    CHECK(core.ChangeCount(Slot) == 0);
}

TEST_CASE("Out of range slots are ignored rather than written past the array")
{
    NamechangerCore core;
    CHECK_FALSE(core.OnIdentity(-1, "name", "", Now).has_value());
    CHECK_FALSE(core.OnIdentity(MaxSlots, "name", "", Now).has_value());
    CHECK(core.ChangeCount(-1) == 0);
}

TEST_CASE("Clan tag changes count exactly like name changes")
{
    NamechangerCore core;
    core.OnBaseline(Slot, "steady", "");

    for (int i = 1; i <= 4; ++i)
        CHECK_FALSE(core.OnIdentity(Slot, "steady", std::format("tag{}", i), Now).has_value());
    CHECK(core.ChangeCount(Slot) == 4);

    const std::optional<Finding> finding = core.OnIdentity(Slot, "steady", "tag5", Now);
    REQUIRE(finding.has_value());
    CHECK(finding->Kind == DetectionKind::Namechanger);
}

TEST_CASE("A repeated read of the same name and tag never counts")
{
    NamechangerCore core;
    core.OnBaseline(Slot, "steady", "[tag]");
    for (int i = 0; i < 100; ++i)
        CHECK_FALSE(core.OnIdentity(Slot, "steady", "[tag]", Now + i * 0.125).has_value());
    CHECK(core.ChangeCount(Slot) == 0);
}

TEST_CASE("After a finding the slot stays quiet for a minute and then counts again")
{
    NamechangerCore core;
    core.OnBaseline(Slot, "steady", "");
    for (int i = 1; i <= 5; ++i)
        core.OnIdentity(Slot, "steady", std::format("tag{}", i), Now);

    for (int i = 6; i <= 40; ++i)
        CHECK_FALSE(core.OnIdentity(Slot, "steady", std::format("tag{}", i), Now + i).has_value());
    CHECK(core.ChangeCount(Slot) == 0);

    std::optional<Finding> finding;
    for (int i = 41; i <= 60 && !finding; ++i)
        finding = core.OnIdentity(Slot, "steady", std::format("tag{}", i), Now + 61.0 + i);
    CHECK(finding.has_value());
}
