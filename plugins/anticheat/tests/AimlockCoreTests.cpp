#include "Core/Geometry.hpp"
#include "Detectors/AimlockCore.hpp"

#include <array>
#include <cmath>
#include <doctest/doctest.h>
#include <limits>

using namespace Anticheat;

namespace
{
constexpr int Observer = 0;
constexpr int Target = 1;
constexpr double Now = 100.0;
constexpr Vec3 Eye{0.0f, 0.0f, 64.0f};
constexpr float TargetX = 500.0f;
constexpr float TargetSpeed = 20.0f;  // units a tick, enough to move the bearing off a stale aim

/**
 * One observer standing still and one enemy walking sideways at a distance. The observer's aim is
 * generated from the enemy's position @p aimLag ticks in the past, which is exactly what a client
 * with that much visual delay would produce.
 */
struct Harness
{
    ShotCorrelatorCore Correlator;
    AimlockCore Aimlock{Correlator};
    LagEstimate Lag = EstimateVisualLag(0.0f, 0.0f);  // one tick of interpolation, no ping
    bool Moving = true;
    int32_t Tick = 0;
    int Findings = 0;
    int32_t FirstFinding = -1;

    explicit Harness(bool moving = true) : Moving(moving)
    {
        // Enough history for every lag hypothesis to have a frame to look back at.
        for (; Tick < 10; ++Tick)
            Correlator.CaptureFrame(Tick, Frame(Tick));
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
        Correlator.CaptureFrame(Tick, Frame(Tick));
        AimAngles aim = Geometry::Bearing(Eye, {TargetX, TargetY(Tick - aimLag), Geometry::BodyHeights[0]});
        aim.Yaw += aimOffsetDeg;
        Aimlock.OnSimulated(Observer, Tick, aim, Eye);
        if (Aimlock.OnFrame(Observer, Tick, true, Lag, Now))
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
}  // namespace

TEST_CASE("EstimateVisualLag adds the full round trip to the client's interpolation delay")
{
    const LagEstimate fast = EstimateVisualLag(0.0f, 0.0f);
    CHECK(fast.Valid);
    CHECK(fast.Ticks == 1);  // a zero interpolation ratio is read as one tick
    CHECK(fast.InterpTicks == doctest::Approx(1.0f));

    const LagEstimate typical = EstimateVisualLag(0.05f, 2.0f);
    CHECK(typical.Valid);
    CHECK(typical.Ticks == 5);  // round(0.05 * 64 + 2)
    CHECK(typical.RoundTripMs == doctest::Approx(50.0f));

    const LagEstimate slow = EstimateVisualLag(0.25f, 2.0f);
    CHECK(slow.Ticks == 18);  // round(16 + 2)
}

TEST_CASE("EstimateVisualLag rejects impossible latency and interpolation values")
{
    CHECK_FALSE(EstimateVisualLag(-0.1f, 2.0f).Valid);
    CHECK_FALSE(EstimateVisualLag(2.1f, 2.0f).Valid);
    CHECK_FALSE(EstimateVisualLag(0.05f, -1.0f).Valid);
    CHECK_FALSE(EstimateVisualLag(0.05f, 19.1f).Valid);
    CHECK(EstimateVisualLag(0.05f, 19.0f).Valid);
    CHECK_FALSE(EstimateVisualLag(std::numeric_limits<float>::quiet_NaN(), 2.0f).Valid);
    CHECK_FALSE(EstimateVisualLag(0.05f, std::numeric_limits<float>::quiet_NaN()).Valid);
}

TEST_CASE("The on target epsilon is the target's own angular half width")
{
    // A 32 unit wide hull measured from its center: the aim may sit anywhere inside it.
    CHECK(Geometry::AngularSizeDeg(16.0f, 500.0f) == doctest::Approx(1.8331f).epsilon(0.001));
    CHECK(Geometry::AngularSizeDeg(16.0f, 2000.0f) == doctest::Approx(0.4584f).epsilon(0.001));
    // The required target travel is one and a half hull widths at the same range.
    CHECK(Geometry::AngularSizeDeg(48.0f, 500.0f) == doctest::Approx(5.4850f).epsilon(0.001));
}

TEST_CASE("Three tracking episodes on a moving target reach the threshold")
{
    Harness harness;
    harness.Run(400);
    CHECK(harness.Findings == 1);
    // Each episode is 96 ticks and the first starts on the first evaluated tick.
    CHECK(harness.FirstFinding >= 290);
    CHECK(harness.FirstFinding <= 310);
}

TEST_CASE("A target that never moves fails the travel gate no matter how precise the aim is")
{
    Harness harness(false);
    harness.Run(400);
    CHECK(harness.Findings == 0);
}

TEST_CASE("An aim keyed to a delay outside the searched hypotheses produces no evidence")
{
    Harness harness;
    // At this range five ticks of stale aim is far wider than the target, so no episode starts at all.
    harness.Run(50, 0, 8);
    CHECK_FALSE(harness.Aimlock.IsTracking(Observer));

    // Further out the same offset does shrink inside the hull, but never for a whole episode.
    harness.Run(350, 0, 8);
    CHECK(harness.Findings == 0);
}

TEST_CASE("An episode with 93 of 97 samples on target still counts")
{
    Harness harness;
    harness.Run(400, 24);  // four skewed samples per episode
    CHECK(harness.Findings == 1);
}

TEST_CASE("An episode with 92 of 97 samples on target falls under the coverage requirement")
{
    Harness harness;
    harness.Run(400, 19);  // five skewed samples per episode
    CHECK(harness.Findings == 0);
}

TEST_CASE("After a detection the module stays quiet until the lock is broken for half a second")
{
    Harness harness;
    harness.Run(400);
    REQUIRE(harness.Findings == 1);
    CHECK_FALSE(harness.Aimlock.IsTracking(Observer));

    // Still glued to the target: latched, so no new episode is even started.
    harness.Run(200);
    CHECK(harness.Findings == 1);
    CHECK_FALSE(harness.Aimlock.IsTracking(Observer));

    // Look away for more than the 32 tick rearm window, then return.
    for (int i = 0; i < 40; ++i)
        harness.Step(90.0f);
    CHECK_FALSE(harness.Aimlock.IsTracking(Observer));

    harness.Step();
    CHECK(harness.Aimlock.IsTracking(Observer));
}

TEST_CASE("A dead or disconnected player drops the tracking state")
{
    Harness harness;
    harness.Run(50);
    REQUIRE(harness.Aimlock.IsTracking(Observer));
    CHECK_FALSE(harness.Aimlock.OnFrame(Observer, harness.Tick, false, harness.Lag, Now).has_value());
    CHECK_FALSE(harness.Aimlock.IsTracking(Observer));
}

TEST_CASE("Episodes counted before a death still count after the respawn")
{
    Harness harness;
    harness.Run(200);  // two full episodes, one short of the threshold
    REQUIRE(harness.Aimlock.IncidentCount(Observer) == 2);
    REQUIRE(harness.Findings == 0);

    // Dying drops the tracking state but must not drop the evidence, or a cheat that dies between
    // episodes never reaches the threshold.
    CHECK_FALSE(harness.Aimlock.OnFrame(Observer, harness.Tick, false, harness.Lag, Now).has_value());
    CHECK_FALSE(harness.Aimlock.IsTracking(Observer));
    CHECK(harness.Aimlock.IncidentCount(Observer) == 2);

    // One more episode after the respawn is the third, so it reports.
    harness.Run(120);
    CHECK(harness.Findings == 1);
}

TEST_CASE("An invalid lag estimate can never start an episode")
{
    Harness harness;
    harness.Lag = EstimateVisualLag(5.0f, 2.0f);  // rejected, so no hypotheses exist
    harness.Run(200);
    CHECK(harness.Findings == 0);
    CHECK_FALSE(harness.Aimlock.IsTracking(Observer));
}

TEST_CASE("A slot change drops the slot's aimlock evidence")
{
    Harness harness;
    harness.Run(200);
    REQUIRE(harness.Aimlock.IncidentCount(Observer) > 0);
    harness.Aimlock.OnSlotChanged(Observer);
    CHECK(harness.Aimlock.IncidentCount(Observer) == 0);
    CHECK_FALSE(harness.Aimlock.IsTracking(Observer));
}
