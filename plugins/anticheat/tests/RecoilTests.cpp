#include "Detect/Rules/Recoil.hpp"

#include <doctest/doctest.h>

using Anticheat::AimAngles;
using Anticheat::CmdSample;
using Anticheat::DefaultTuning;
using Anticheat::DetectionKind;
using Anticheat::Rules::Recoil;
using Anticheat::ShotView;
using Anticheat::Suspicion;

static constexpr int Slot = 3;
static constexpr double Now = 100.0;
static constexpr int CommandsPerShot = 6;

/**
 * One player spraying at a wall. The recoil kicks the punch on every shot; the view moves against
 * it by @p factor, one command later, with an optional per-shot error the way a hand would.
 */
struct RecoilHarness
{
    RecoilHarness() { Scores.Configure(DefaultTuning()); }

    /** Sprays this rule has marked on the slot. */
    float Marked() const { return Scores.Value(Slot, DetectionKind::Recoil, Now); }

    Suspicion Scores;
    Recoil Rule{Scores};
    int32_t Cmd = 100;
    AimAngles View{10.0f, 0.0f};
    AimAngles Punch;
    int Findings = 0;

    void Command()
    {
        CmdSample cmd;
        cmd.CmdNum = Cmd;
        cmd.ClientTick = Cmd;
        cmd.ViewPitch = View.Pitch;
        cmd.ViewYaw = View.Yaw;
        cmd.HasPunch = true;
        cmd.Punch = Punch;
        Rule.OnCommand(Slot, cmd);
        ++Cmd;
    }

    void Spray(int shots, float factor, float handError, int viewLag = 1, const char* weapon = "ak47")
    {
        AimAngles pendingKick;
        int pendingFor = 0;
        for (int shot = 1; shot <= shots; ++shot)
        {
            for (int step = 0; step < CommandsPerShot; ++step)
            {
                const bool fire = step == 0;
                // Alternate the hand's error so no constant factor can absorb it.
                const float error = shot % 2 == 0 ? 1.0f + handError : 1.0f - handError;
                const AimAngles kick{-1.2f, shot % 2 == 0 ? 0.4f : -0.4f};
                if (fire && viewLag == 0)
                {
                    View.Pitch -= factor * kick.Pitch * error;
                    View.Yaw -= factor * kick.Yaw * error;
                }
                if (pendingFor > 0 && --pendingFor == 0)
                {
                    View.Pitch -= factor * pendingKick.Pitch;
                    View.Yaw -= factor * pendingKick.Yaw;
                }
                if (fire)
                {
                    ShotView view;
                    view.Slot = Slot;
                    view.CmdNum = Cmd;
                    view.FireTick = Cmd;
                    view.Weapon = weapon;
                    view.ShotsFired = shot;
                    if (Rule.OnShot(Slot, view, Now))
                        ++Findings;
                    // The kick is in the punch the next command carries.
                    Punch.Pitch += kick.Pitch;
                    Punch.Yaw += kick.Yaw;
                    if (viewLag == 1)
                    {
                        pendingKick = {kick.Pitch * error, kick.Yaw * error};
                        pendingFor = 1;
                    }
                }
                Command();
            }
        }
        Punch = {};
    }

    void Quiet()
    {
        Cmd += 40;
        if (Rule.OnFrame(Slot, Cmd, Now))
            ++Findings;
    }
};

TEST_CASE("Every spray whose view cancels the punch is marked, and three of them report")
{
    RecoilHarness h;
    for (int spray = 0; spray < 2; ++spray)
    {
        h.Spray(10, 1.0f, 0.0f);
        h.Quiet();
    }
    CHECK(h.Findings == 0);
    CHECK(h.Marked() == doctest::Approx(2.0f));

    h.Spray(10, 1.0f, 0.0f);
    h.Quiet();
    CHECK(h.Findings == 1);
    CHECK(h.Marked() == doctest::Approx(3.0f));
}

TEST_CASE("A hand that follows the pattern with its own error is not recoil control")
{
    RecoilHarness h;
    for (int spray = 0; spray < 3; ++spray)
    {
        h.Spray(10, 1.0f, 0.5f);
        h.Quiet();
    }
    CHECK(h.Findings == 0);
    CHECK(h.Marked() == doctest::Approx(0.0f));
}

TEST_CASE("Cancelling in the same command and a scaled factor both fit")
{
    RecoilHarness h;
    for (int spray = 0; spray < 3; ++spray)
    {
        h.Spray(10, 0.8f, 0.0f, 0);
        h.Quiet();
    }
    CHECK(h.Findings == 1);
}

TEST_CASE("Short bursts carry too little punch to judge")
{
    RecoilHarness h;
    for (int spray = 0; spray < 4; ++spray)
    {
        h.Spray(5, 1.0f, 0.0f);
        h.Quiet();
    }
    CHECK(h.Marked() == doctest::Approx(0.0f));
}

TEST_CASE("A new burst counter closes the spray before it")
{
    RecoilHarness h;
    h.Spray(10, 1.0f, 0.0f);
    CHECK(h.Rule.InSpray(Slot));
    // Restarting at shot one without a quiet gap still ends the previous spray.
    h.Spray(1, 1.0f, 0.0f);
    CHECK(h.Marked() == doctest::Approx(1.0f));
}
