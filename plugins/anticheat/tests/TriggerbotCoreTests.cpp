#include "Detect/Rules/TriggerbotCore.hpp"
#include "Detect/Geometry.hpp"

#include <array>
#include <doctest/doctest.h>

using Anticheat::AimAngles;
using Anticheat::EstimateVisualLag;
using Anticheat::LagEstimate;
using Anticheat::MaxSlots;
using Anticheat::PositionSample;
using Anticheat::ShotCorrelatorCore;
using Anticheat::ShotView;
using Anticheat::TeamCT;
using Anticheat::TeamT;
using Anticheat::TriggerbotCore;
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
    ShotCorrelatorCore Correlator;
    TriggerbotCore Core{Correlator};
    LagEstimate Lag = EstimateVisualLag(0.0f, 0.0f);
    int32_t Tick = 0;
    float TargetY = -300.0f;
    AimAngles Aim = Geometry::Bearing(Eye, {TargetX, 0.0f, Geometry::BodyHeights[1]});
    int Findings = 0;

    TriggerbotHarness()
    {
        for (; Tick < 8; ++Tick)
            Correlator.CaptureFrame(Tick, Frame());
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
        Correlator.CaptureFrame(Tick, Frame());
        Core.OnSimulated(Observer, Tick, Aim, Eye);
        Core.OnFrame(Observer, Tick, true, Lag);
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
        Core.OnWeaponFire(Observer, fireTick);
        ShotView shot;
        shot.Slot = Observer;
        shot.FireTick = fireTick;
        shot.HurtSeen = true;
        shot.VictimSlot = Target;
        if (Core.OnPlayerHurt(Observer, shot, Now))
            ++Findings;
    }
};

TEST_CASE("Hits one tick after the enemy walks into a resting crosshair add up to a finding")
{
    TriggerbotHarness h;
    for (int episode = 0; episode < 3; ++episode)
    {
        h.Approach();
        h.Hit(h.Tick);
        CHECK(h.Findings == 0);
    }
    CHECK(h.Core.Score(Observer, Now) == 6);
    h.Approach();
    h.Hit(h.Tick);
    CHECK(h.Findings == 1);
    CHECK(h.Core.Score(Observer, Now) == 0);
}

TEST_CASE("A human reaction after the enemy stopped under the crosshair is not evidence")
{
    TriggerbotHarness h;
    h.Approach();
    for (int i = 0; i < 12; ++i)
        h.Step(false);
    h.Hit(h.Tick);
    CHECK(h.Core.Score(Observer, Now) == 0);
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
    CHECK(h.Core.Score(Observer, Now) == 0);
}

TEST_CASE("A shot inside a burst is not judged as a reaction")
{
    TriggerbotHarness h;
    h.Approach();
    h.Hit(h.Tick);
    CHECK(h.Core.Score(Observer, Now) == 2);

    h.Approach();
    h.Core.OnWeaponFire(Observer, h.Tick - 3);  // a miss three ticks earlier
    h.Hit(h.Tick);
    CHECK(h.Core.Score(Observer, Now) == 2);
}

TEST_CASE("Without a usable lag estimate nothing is counted")
{
    TriggerbotHarness h;
    h.Lag = {};
    h.Approach();
    h.Hit(h.Tick);
    CHECK(h.Core.Score(Observer, Now) == 0);
}
