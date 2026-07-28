#include "Detectors/NamechangerCore.hpp"

#include <doctest/doctest.h>
#include <format>

using namespace Anticheat;

namespace
{
constexpr int Slot = 3;
constexpr double Now = 100.0;
}  // namespace

TEST_CASE("The fifth name change inside one minute fires and the fourth does not")
{
    NamechangerCore core;
    core.OnBaseline(Slot, "original");

    for (int i = 1; i <= 4; ++i)
        CHECK_FALSE(core.OnNameChanged(Slot, std::format("name{}", i), Now).has_value());
    CHECK(core.ChangeCount(Slot) == 4);

    const std::optional<Finding> finding = core.OnNameChanged(Slot, "name5", Now);
    REQUIRE(finding.has_value());
    CHECK(finding->Kind == DetectionKind::Namechanger);
    CHECK_FALSE(finding->KickOnly);
    CHECK(core.ChangeCount(Slot) == 0);  // firing clears the window
}

TEST_CASE("A settings change that leaves the name alone is not a change")
{
    NamechangerCore core;
    core.OnBaseline(Slot, "steady");
    for (int i = 0; i < 10; ++i)
        CHECK_FALSE(core.OnNameChanged(Slot, "steady", Now).has_value());
    CHECK(core.ChangeCount(Slot) == 0);
}

TEST_CASE("Changes older than a minute fall out of the rolling window")
{
    NamechangerCore core;
    core.OnBaseline(Slot, "original");
    for (int i = 1; i <= 4; ++i)
        core.OnNameChanged(Slot, std::format("name{}", i), Now);
    CHECK(core.ChangeCount(Slot) == 4);

    // A minute later the earlier four are gone, so this one stands alone.
    CHECK_FALSE(core.OnNameChanged(Slot, "name5", Now + 61.0).has_value());
    CHECK(core.ChangeCount(Slot) == 1);
}

TEST_CASE("A change arriving before the baseline establishes it instead of counting")
{
    NamechangerCore core;
    CHECK_FALSE(core.OnNameChanged(Slot, "first", Now).has_value());
    CHECK(core.ChangeCount(Slot) == 0);
    CHECK_FALSE(core.OnNameChanged(Slot, "first", Now).has_value());
    CHECK(core.ChangeCount(Slot) == 0);
    core.OnNameChanged(Slot, "second", Now);
    CHECK(core.ChangeCount(Slot) == 1);
}

TEST_CASE("An empty name is ignored rather than counted as a change")
{
    NamechangerCore core;
    core.OnBaseline(Slot, "original");
    CHECK_FALSE(core.OnNameChanged(Slot, "", Now).has_value());
    CHECK(core.ChangeCount(Slot) == 0);
}

TEST_CASE("A new baseline drops the slot's previous change history")
{
    NamechangerCore core;
    core.OnBaseline(Slot, "original");
    for (int i = 1; i <= 4; ++i)
        core.OnNameChanged(Slot, std::format("name{}", i), Now);
    core.OnBaseline(Slot, "rejoined");
    CHECK(core.ChangeCount(Slot) == 0);
    CHECK_FALSE(core.OnNameChanged(Slot, "rejoined", Now).has_value());
}

TEST_CASE("A slot change and a reset both clear the history")
{
    NamechangerCore core;
    core.OnBaseline(Slot, "original");
    for (int i = 1; i <= 4; ++i)
        core.OnNameChanged(Slot, std::format("name{}", i), Now);

    core.OnSlotChanged(Slot);
    CHECK(core.ChangeCount(Slot) == 0);

    core.OnBaseline(Slot, "original");
    for (int i = 1; i <= 4; ++i)
        core.OnNameChanged(Slot, std::format("name{}", i), Now);
    core.Reset();
    CHECK(core.ChangeCount(Slot) == 0);
}

TEST_CASE("Out of range slots are ignored rather than written past the array")
{
    NamechangerCore core;
    CHECK_FALSE(core.OnNameChanged(-1, "name", Now).has_value());
    CHECK_FALSE(core.OnNameChanged(MaxSlots, "name", Now).has_value());
    CHECK(core.ChangeCount(-1) == 0);
}
