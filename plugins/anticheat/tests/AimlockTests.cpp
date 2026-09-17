#include "Detect/Geometry.hpp"
#include "Detect/Rules/Aimlock.hpp"

#include <array>
#include <cmath>
#include <doctest/doctest.h>
#include <limits>

using Anticheat::AimAngles;
using Anticheat::DetectionKind;
using Anticheat::Rules::Aimlock;
using Anticheat::EstimateViewLag;
using Anticheat::ViewLag;
using Anticheat::MaxSlots;
using Anticheat::PositionSample;
using Anticheat::ShotHistory;
using Anticheat::Suspicion;
using Anticheat::TeamCT;
using Anticheat::TeamT;
using Anticheat::Vec3;
namespace Geometry = Anticheat::Geometry;

static constexpr int Observer = 0;
static constexpr int Target = 1;
static constexpr double Now = 100.0;
static constexpr Vec3 Eye{0.0f, 0.0f, 64.0f};
static constexpr float TargetX = 500.0f;
static constexpr float TargetSpeed = 20.0f;  // units a tick, enough to move the bearing off a stale aim

/**
 * One observer standing still and one enemy walking sideways at a distance. The aim is generated
 * from the enemy's position @p aimLag ticks in the past - what a client with that much visual
 * delay would produce.
 */
struct AimlockHarness
{
    Suspicion Scores;
    ShotHistory History;
    Aimlock Rule{History, Scores};
    ViewLag Lag = EstimateViewLag(0.0f, 0.0f);  // one tick of interpolation, no ping
    bool Moving = true;
    int32_t Tick = 0;
    int Findings = 0;
    int32_t FirstFinding = -1;

    /** Tracking episodes counted against the observer. */
    float Episodes() const { return Scores.Value(Observer, DetectionKind::Aimlock, Now) * 3.0f; }

    explicit AimlockHarness(bool moving = true) : Moving(moving)
    {
        // Enough history for every lag hypothesis to have a frame to look back at.
        for (; Tick < 10; ++Tick)
            History.CaptureFrame(Tick, Frame(Tick));
    }

    float TargetY(int32_t tick) const { return Moving ? TargetSpeed * static_cast<float>(tick) : 0.0f; }

    std::array<PositionSample, MaxSlots> Frame(int32_t tick) const
    {
        std::array<PositionSample, MaxSlots> players{};
        players[Observer] = {.Origin = {0.0f, 0.0f, 0.0f}, .EyePos = Eye, .Team = TeamT, .Valid = true, .Alive = true};
        players[Target] = {.Origin = {TargetX, TargetY(tick), 0.0f},
                           .EyePos = {TargetX, TargetY(tick), 64.0f},
                           .Team = TeamCT,
                           .Valid = true,
                           .Alive = true};
        return players;
    }

    void Step(float aimOffsetDeg = 0.0f, int aimLag = 1)
    {
        History.CaptureFrame(Tick, Frame(Tick));
        AimAngles aim = Geometry::Bearing(Eye, {TargetX, TargetY(Tick - aimLag), Geometry::BodyHeights[0]});
        aim.Yaw += aimOffsetDeg;
        Rule.OnSimulated(Observer, Tick, aim, Eye);
        if (Rule.OnFrame(Observer, Tick, true, Lag, Now))
        {
            ++Findings;
            if (FirstFinding < 0)
                FirstFinding = Tick;
        }
        ++Tick;
    }

    void Run(int ticks, int skewEvery = 0, int aimLag = 1)
    {
        for (int i = 0; i < ticks; ++i)
            Step(skewEvery > 0 && Tick % skewEvery == 0 ? 10.0f : 0.0f, aimLag);
    }
};

TEST_CASE("EstimateViewLag adds the full round trip to the client's interpolation delay")
{
    const ViewLag fast = EstimateViewLag(0.0f, 0.0f);
    CHECK(fast.Valid);
    CHECK(fast.Ticks == 1);  // a zero interpolation ratio is read as one tick
    CHECK(fast.InterpTicks == doctest::Approx(1.0f));

    const ViewLag typical = EstimateViewLag(0.05f, 2.0f);
    CHECK(typical.Valid);
    CHECK(typical.Ticks == 5);  // round(0.05 * 64 + 2)
    CHECK(typical.RoundTripMs == doctest::Approx(50.0f));

    const ViewLag slow = EstimateViewLag(0.25f, 2.0f);
    CHECK(slow.Ticks == 18);  // round(16 + 2)
}

TEST_CASE("EstimateViewLag rejects impossible latency and interpolation values")
{
    CHECK_FALSE(EstimateViewLag(-0.1f, 2.0f).Valid);
    CHECK_FALSE(EstimateViewLag(2.1f, 2.0f).Valid);
    CHECK_FALSE(EstimateViewLag(0.05f, -1.0f).Valid);
    CHECK_FALSE(EstimateViewLag(0.05f, 19.1f).Valid);
    CHECK(EstimateViewLag(0.05f, 19.0f).Valid);
    CHECK_FALSE(EstimateViewLag(std::numeric_limits<float>::quiet_NaN(), 2.0f).Valid);
    CHECK_FALSE(EstimateViewLag(0.05f, std::numeric_limits<float>::quiet_NaN()).Valid);
}

TEST_CASE("Three tracking episodes on a moving target reach the threshold")
{
    AimlockHarness harness;
    harness.Run(400);
    CHECK(harness.Findings == 1);
    // Each episode is 96 ticks and the first starts on the first evaluated tick.
    CHECK(harness.FirstFinding >= 290);
    CHECK(harness.FirstFinding <= 310);
}

TEST_CASE("A target that never moves fails the travel gate no matter how precise the aim is")
{
    AimlockHarness harness(false);
    harness.Run(400);
    CHECK(harness.Findings == 0);
}

TEST_CASE("An aim keyed to a delay outside the searched hypotheses produces no evidence")
{
    AimlockHarness harness;
    // At this range five ticks of stale aim is far wider than the target, so no episode starts at all.
    harness.Run(50, 0, 8);
    CHECK_FALSE(harness.Rule.IsTracking(Observer));

    // Further out the same offset does shrink inside the hull, but never for a whole episode.
    harness.Run(350, 0, 8);
    CHECK(harness.Findings == 0);
}

TEST_CASE("An episode with 93 of 97 samples on target still counts")
{
    AimlockHarness harness;
    harness.Run(400, 24);  // four skewed samples per episode
    CHECK(harness.Findings == 1);
}

TEST_CASE("An episode with 92 of 97 samples on target falls under the coverage requirement")
{
    AimlockHarness harness;
    harness.Run(400, 19);  // five skewed samples per episode
    CHECK(harness.Findings == 0);
}

TEST_CASE("After a detection the module stays quiet until the lock is broken for half a second")
{
    AimlockHarness harness;
    harness.Run(400);
    REQUIRE(harness.Findings == 1);
    CHECK_FALSE(harness.Rule.IsTracking(Observer));

    // Still glued to the target: still locked, so no new episode is even started.
    harness.Run(200);
    CHECK(harness.Findings == 1);
    CHECK_FALSE(harness.Rule.IsTracking(Observer));

    // Look away for longer than the 32 tick off-target window, then return.
    for (int i = 0; i < 40; ++i)
        harness.Step(90.0f);
    CHECK_FALSE(harness.Rule.IsTracking(Observer));

    harness.Step();
    CHECK(harness.Rule.IsTracking(Observer));
}

TEST_CASE("A dead or disconnected player drops the tracking state")
{
    AimlockHarness harness;
    harness.Run(50);
    REQUIRE(harness.Rule.IsTracking(Observer));
    CHECK_FALSE(harness.Rule.OnFrame(Observer, harness.Tick, false, harness.Lag, Now).has_value());
    CHECK_FALSE(harness.Rule.IsTracking(Observer));
}

TEST_CASE("Episodes counted before a death still count after the respawn")
{
    AimlockHarness harness;
    harness.Run(200);  // two full episodes, one short of the threshold
    REQUIRE(harness.Episodes() == doctest::Approx(2.0f));
    REQUIRE(harness.Findings == 0);

    // Dying drops the tracking state but not the evidence, or a cheat that dies between episodes
    // never reaches the threshold.
    CHECK_FALSE(harness.Rule.OnFrame(Observer, harness.Tick, false, harness.Lag, Now).has_value());
    CHECK_FALSE(harness.Rule.IsTracking(Observer));
    CHECK(harness.Episodes() == doctest::Approx(2.0f));

    // One more episode after the respawn is the third, so it reports.
    harness.Run(120);
    CHECK(harness.Findings == 1);
}

TEST_CASE("An invalid lag estimate can never start an episode")
{
    AimlockHarness harness;
    harness.Lag = EstimateViewLag(5.0f, 2.0f);  // rejected, so no hypotheses exist
    harness.Run(200);
    CHECK(harness.Findings == 0);
    CHECK_FALSE(harness.Rule.IsTracking(Observer));
}

TEST_CASE("A slot change drops the slot's aimlock evidence")
{
    AimlockHarness harness;
    harness.Run(200);
    REQUIRE(harness.Episodes() > 0.0f);
    harness.Rule.OnSlotChanged(Observer);
    harness.Scores.OnSlotChanged(Observer);
    CHECK(harness.Episodes() == doctest::Approx(0.0f));
    CHECK_FALSE(harness.Rule.IsTracking(Observer));
}
