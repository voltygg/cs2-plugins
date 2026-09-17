#include "Detect/Rules/Triggerbot.hpp"
#include "Detect/Geometry.hpp"

#include <array>
#include <doctest/doctest.h>

using Anticheat::AimAngles;
using Anticheat::DetectionKind;
using Anticheat::EstimateViewLag;
using Anticheat::ViewLag;
using Anticheat::MaxSlots;
using Anticheat::PositionSample;
using Anticheat::ShotHistory;
using Anticheat::ShotView;
using Anticheat::TeamCT;
using Anticheat::TeamT;
using Anticheat::Rules::Triggerbot;
using Anticheat::Suspicion;
using Anticheat::Vec3;
namespace Geometry = Anticheat::Geometry;

static constexpr int Observer = 0;
static constexpr int Target = 1;
static constexpr double Now = 100.0;
static constexpr Vec3 Eye{0.0f, 0.0f, 64.0f};
static constexpr float TargetX = 500.0f;
/** Big enough that the hull edge is never landed on exactly: -25 is off, 0 is on. */
static constexpr float TargetSpeed = 25.0f;

/** A crosshair resting on the spot an enemy walks through, from the side. */
struct TriggerbotHarness
{
    Suspicion Scores;
    ShotHistory History;
    Triggerbot Rule{History, Scores};
    ViewLag Lag = EstimateViewLag(0.0f, 0.0f);
    int32_t Tick = 0;
    float TargetY = -300.0f;
    AimAngles Aim = Geometry::Bearing(Eye, {TargetX, 0.0f, Geometry::BodyHeights[1]});
    int Findings = 0;

    /** Points this rule has on the observer, in its own units, where eight weighted points are one whole unit of suspicion. */
    float Points() const { return Scores.Value(Observer, DetectionKind::Triggerbot, Now) * 8.0f; }

    TriggerbotHarness()
    {
        Scores.Configure({});
        for (; Tick < 8; ++Tick)
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
                           .Alive = true};
        return players;
    }

    void Step(bool moving = true)
    {
        if (moving)
            TargetY += TargetSpeed;
        History.CaptureFrame(Tick, Frame());
        Rule.OnSimulated(Observer, Tick, Aim, Eye);
        Rule.OnFrame(Observer, Tick, true, Lag);
        ++Tick;
    }

    /** Walk the enemy in from the side until it stands under the crosshair. */
    void Approach()
    {
        TargetY = -300.0f;
        while (TargetY < -1.0f)
            Step();
    }

    void Hit(int32_t fireTick)
    {
        Rule.OnWeaponFire(Observer, fireTick);
        ShotView shot;
        shot.Slot = Observer;
        shot.FireTick = fireTick;
        shot.HurtSeen = true;
        shot.VictimSlot = Target;
        if (Rule.OnPlayerHurt(Observer, shot, Now))
            ++Findings;
    }
};

TEST_CASE("A hit one tick after the enemy walks into a resting crosshair is worth two points")
{
    TriggerbotHarness h;
    for (int episode = 0; episode < 3; ++episode)
    {
        h.Approach();
        h.Hit(h.Tick);
        CHECK(h.Findings == 0);
    }
    CHECK(h.Points() == doctest::Approx(6.0f));

    // Eight points are what this rule reports on alone; the score decides that, not the rule.
    h.Approach();
    h.Hit(h.Tick);
    CHECK(h.Findings == 1);
    CHECK(h.Points() == doctest::Approx(8.0f));
}

TEST_CASE("A human reaction after the enemy stopped under the crosshair is not evidence")
{
    TriggerbotHarness h;
    h.Approach();
    for (int i = 0; i < 12; ++i)
        h.Step(false);
    h.Hit(h.Tick);
    CHECK(h.Points() == doctest::Approx(0.0f));
}

TEST_CASE("A flick onto a standing enemy followed by a shot is aim, not a trigger")
{
    TriggerbotHarness h;
    h.TargetY = 0.0f;
    const AimAngles onTarget = h.Aim;
    h.Aim.Yaw += 10.0f;
    for (int i = 0; i < 6; ++i)
        h.Step(false);
    h.Aim = onTarget;
    h.Step(false);
    h.Hit(h.Tick);
    CHECK(h.Points() == doctest::Approx(0.0f));
}

TEST_CASE("A shot inside a burst is not judged as a reaction")
{
    TriggerbotHarness h;
    h.Approach();
    h.Hit(h.Tick);
    CHECK(h.Points() == doctest::Approx(2.0f));

    h.Approach();
    h.Rule.OnWeaponFire(Observer, h.Tick - 3);  // a miss three ticks earlier
    h.Hit(h.Tick);
    CHECK(h.Points() == doctest::Approx(2.0f));
}

TEST_CASE("Without a usable lag estimate nothing is counted")
{
    TriggerbotHarness h;
    h.Lag = {};
    h.Approach();
    h.Hit(h.Tick);
    CHECK(h.Points() == doctest::Approx(0.0f));
}
