#include "Detect/Rules/RecoilCore.hpp"

#include <doctest/doctest.h>

using Anticheat::AimAngles;
using Anticheat::CmdSample;
using Anticheat::RecoilCore;
using Anticheat::ShotView;

static constexpr int Slot = 3;
static constexpr double Now = 100.0;
static constexpr int CommandsPerShot = 6;

/**
 * One player spraying at a wall. The recoil kicks the punch on every shot; the view moves against
 * it by @p factor, one command later, with an optional per-shot error the way a hand would.
 */
struct RecoilHarness
{
    RecoilCore Core;
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
        Core.OnCommand(Slot, cmd);
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
                    if (Core.OnShot(Slot, view, Now))
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
        if (Core.OnFrame(Slot, Cmd, Now))
            ++Findings;
    }
};

TEST_CASE("Sprays whose view cancels the punch exactly add up to a finding")
{
    RecoilHarness h;
    for (int spray = 0; spray < 2; ++spray)
    {
        h.Spray(10, 1.0f, 0.0f);
        h.Quiet();
    }
    CHECK(h.Findings == 0);
    CHECK(h.Core.Score(Slot, Now) == 2);
    h.Spray(10, 1.0f, 0.0f);
    h.Quiet();
    CHECK(h.Findings == 1);
    CHECK(h.Core.Score(Slot, Now) == 0);
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
    CHECK(h.Core.Score(Slot, Now) == 0);
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
    CHECK(h.Core.Score(Slot, Now) == 0);
}

TEST_CASE("A new burst counter closes the spray before it")
{
    RecoilHarness h;
    h.Spray(10, 1.0f, 0.0f);
    CHECK(h.Core.InSpray(Slot));
    // Restarting at shot one without a quiet gap still ends the previous spray.
    h.Spray(1, 1.0f, 0.0f);
    CHECK(h.Core.Score(Slot, Now) == 1);
}
