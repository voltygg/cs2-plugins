#include "Detect/Rules/Wallhack.hpp"
#include "Detect/Geometry.hpp"
#include "Harness.hpp"

#include <array>
#include <doctest/doctest.h>
#include <optional>

using Anticheat::AimAngles;
using Anticheat::DetectionKind;
using Anticheat::EstimateViewLag;
using Anticheat::ViewLag;
using Anticheat::MaxSlots;
using Anticheat::PositionSample;
using Anticheat::ShotHistory;
using Anticheat::ShotView;
using Anticheat::SlotBit;
using Anticheat::Suspicion;
using Anticheat::TeamCT;
using Anticheat::TeamT;
using Anticheat::Vec3;
using Anticheat::Rules::Wallhack;
using Anticheat::Rules::WallhackShotContext;
namespace Geometry = Anticheat::Geometry;

static constexpr int Observer = 0;
static constexpr int Target = 1;
static constexpr double Now = 100.0;
static constexpr Vec3 Eye{0.0f, 0.0f, 64.0f};
static constexpr float TargetX = 500.0f;

/** One observer and one enemy walking sideways behind a wall, with the sight line stamped. */
struct WallhackHarness
{
    Anticheat::Test::Findings Reported;
    Suspicion Scores;
    ShotHistory History;
    Wallhack Rule{History, Scores};
    ViewLag Lag = EstimateViewLag(0.0f, 0.0f);  // one tick behind
    int32_t Tick = 0;
    float TargetY = -128.0f;
    float TargetSpeed = 4.0f;
    bool Hidden = true;
    bool Known = true;
    int Findings = 0;

    /** Points this rule has on the observer, in its own units, where six weighted points are one whole unit of suspicion. */
    float Points() const { return Scores.Value(Observer, DetectionKind::Wallhack, Now) * 6.0f; }

    WallhackHarness()
    {
        Scores.ReportTo(Reported.Sink());
        for (; Tick < 4; ++Tick)
            History.CaptureFrame(Tick, Frame());
    }

    std::array<PositionSample, MaxSlots> Frame() const
    {
        std::array<PositionSample, MaxSlots> players{};
        players[Observer] = {.Origin = {0.0f, 0.0f, 0.0f}, .EyePos = Eye, .Team = TeamT, .Valid = true, .Alive = true};
        players[Target] = {.Origin = {TargetX, TargetY, 0.0f},
                           .EyePos = {TargetX, TargetY, 64.0f},
                           .Team = TeamCT,
                           .Valid = true,
                           .Alive = true,
                           .CheckedBy = Known ? SlotBit(Observer) : 0,
                           .SeenBy = Known && !Hidden ? SlotBit(Observer) : 0};
        return players;
    }

    AimAngles Following() const
    {
        // Where the client saw the enemy: one tick behind.
        return Geometry::Bearing(Eye, {TargetX, TargetY - TargetSpeed, Geometry::BodyHeights[1]});
    }

    void Step(const AimAngles& aim)
    {
        TargetY += TargetSpeed;
        History.CaptureFrame(Tick, Frame());
        Rule.OnSimulated(Observer, Tick, aim, Eye);
        Rule.OnFrame(Observer, Tick, true, Lag, Now);
        Findings = Reported.Count;
        ++Tick;
    }

    void Follow(int ticks)
    {
        for (int i = 0; i < ticks; ++i)
            Step(Following());
    }

    /** An enemy in the open for a while, aimed nowhere near, then hidden again on the far side. */
    void Break()
    {
        Hidden = false;
        for (int i = 0; i < 70; ++i)
            Step({0.0f, 90.0f});
        Hidden = true;
        TargetY = -128.0f;
    }

    std::optional<Anticheat::Finding> Hit(int32_t fireTick, bool headshot = false, bool teamSaw = false)
    {
        ShotView shot;
        shot.Slot = Observer;
        shot.FireTick = fireTick;
        shot.HurtSeen = true;
        shot.Headshot = headshot;
        shot.VictimSlot = Target;
        Reported.Last.reset();
        Rule.OnShot(Observer, shot, WallhackShotContext{.TeamSawVictim = teamSaw}, Now);
        return Reported.Last;
    }
};

TEST_CASE("Following a hidden enemy through a wall for a second is an episode worth two points")
{
    WallhackHarness h;
    h.Follow(70);
    CHECK(h.Points() == doctest::Approx(2.0f));
    CHECK(h.Rule.IsTracking(Observer));

    // Three episodes are what this rule reports on alone; the score decides that, not the rule.
    h.Break();
    h.Follow(70);
    h.Break();
    h.Follow(70);
    CHECK(h.Points() == doctest::Approx(6.0f));
    CHECK(h.Findings == 1);
}

TEST_CASE("A crosshair resting where a hidden enemy happens to pass is not following")
{
    WallhackHarness h;
    const AimAngles resting = Geometry::Bearing(Eye, {TargetX, 0.0f, Geometry::BodyHeights[1]});
    for (int i = 0; i < 70; ++i)
        h.Step(resting);
    CHECK(h.Points() == doctest::Approx(0.0f));
}

TEST_CASE("Following an enemy in plain view is aim, not a wallhack")
{
    WallhackHarness h;
    h.Hidden = false;
    h.Follow(70);
    CHECK(h.Points() == doctest::Approx(0.0f));
    CHECK_FALSE(h.Rule.IsTracking(Observer));
}

TEST_CASE("Sight lines that were never traced count for nothing")
{
    WallhackHarness h;
    h.Known = false;
    h.Follow(70);
    CHECK(h.Points() == doctest::Approx(0.0f));
}

TEST_CASE("A hit right after following the enemy into view is evidence")
{
    WallhackHarness h;
    h.Follow(30);
    h.Hidden = false;
    h.Step(h.Following());
    CHECK_FALSE(h.Rule.IsTracking(Observer));
    h.Hit(h.Tick + 3);
    CHECK(h.Points() == doctest::Approx(2.0f));
}

TEST_CASE("A shot through cover at a silent, unseen, unmoving enemy is evidence")
{
    WallhackHarness h;
    h.TargetSpeed = 0.0f;
    for (int i = 0; i < 20; ++i)
        h.Step({0.0f, 90.0f});
    const int32_t fireTick = h.Tick - 1;

    SUBCASE("body shot")
    {
        h.Hit(fireTick);
        CHECK(h.Points() == doctest::Approx(2.0f));
    }
    SUBCASE("headshot")
    {
        h.Hit(fireTick, true);
        CHECK(h.Points() == doctest::Approx(3.0f));
    }
    SUBCASE("a teammate could see the victim")
    {
        h.Hit(fireTick, false, true);
        CHECK(h.Points() == doctest::Approx(0.0f));
    }
    SUBCASE("the victim fired recently")
    {
        h.Rule.OnWeaponFire(Target, fireTick - 10);
        h.Hit(fireTick);
        CHECK(h.Points() == doctest::Approx(0.0f));
    }
}

TEST_CASE("A running enemy behind cover was heard, so the shot is not evidence")
{
    WallhackHarness h;
    h.TargetSpeed = 4.0f;  // 256 units a second
    for (int i = 0; i < 20; ++i)
        h.Step({0.0f, 90.0f});
    h.Hit(h.Tick - 1);
    CHECK(h.Points() == doctest::Approx(0.0f));
}
