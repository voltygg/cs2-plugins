#include "Config.hpp"
#include "Detectors/AimSnapCore.hpp"
#include "Detectors/SpinbotDetector.hpp"
#include "MicroTest.hpp"
#include "PlayerMonitor.hpp"

#include <CS2Kit/Sdk/UserCmd.hpp>
#include <CS2Kit/Utils/DecayingScore.hpp>
#include <array>
#include <cmath>
#include <cstring>
#include <string>

using namespace Anticheat;
using CS2Kit::Sdk::UserCmdView;
using CS2Kit::Utils::DecayingScore;

namespace
{
bool Near(float a, float b, float eps = 0.1f)
{
    return std::fabs(a - b) < eps;
}

UserCmdView Cmd(float yaw, float pitch = 0.0f)
{
    UserCmdView c;
    c.Valid = true;
    c.ViewYaw = yaw;
    c.ViewPitch = pitch;
    return c;
}

// Score at the moment of the Nth event, events spaced `spacing` seconds apart.
float ValueAtNthEvent(int n, double spacing, float eventScore, float decayPerSec)
{
    DecayingScore score(decayPerSec);
    for (int i = 0; i < n; ++i)
        score.Add(eventScore, i * spacing);
    return score.Value((n - 1) * spacing);
}
}  // namespace

// --- B1: AimSnap masking regression -----------------------------------------

TEST_CASE("FindSettledSnap detects the snap nearest the shot despite a larger earlier motion")
{
    AimSnapSettings cfg;  // settleEpsilonDeg=1, minSnapDeg=40
    std::array<UserCmdView, 7> w = {
        Cmd(100.0f),  // ago 0: fire tick (after the 45 deg snap)
        Cmd(55.0f),   // ago 1: 45 deg step -> the aimbot snap
        Cmd(55.0f),   // ago 2..4: locked
        Cmd(55.0f),  Cmd(55.0f),
        Cmd(-5.0f),  // ago 5: 60 deg step -> larger, but earlier, legit motion
        Cmd(-5.0f),
    };
    auto snap = Detectors::AimSnap::FindSettledSnap(w, cfg);
    CHECK(snap.has_value());
    if (snap)
    {
        CHECK_EQ(snap->Ago, 1);
        CHECK(Near(snap->SnapDeg, 45.0f));
    }
}

TEST_CASE("FindSettledSnap finds a clean snap-and-lock several ticks before the shot")
{
    AimSnapSettings cfg;
    std::array<UserCmdView, 6> w = {Cmd(100.0f), Cmd(100.0f), Cmd(100.0f), Cmd(50.0f), Cmd(50.0f), Cmd(50.0f)};
    auto snap = Detectors::AimSnap::FindSettledSnap(w, cfg);
    CHECK(snap.has_value());
    if (snap)
        CHECK_EQ(snap->Ago, 3);
}

TEST_CASE("FindSettledSnap ignores sub-minSnap motion and pure stillness")
{
    AimSnapSettings cfg;
    std::array<UserCmdView, 5> still = {Cmd(10.0f), Cmd(10.0f), Cmd(10.0f), Cmd(10.0f), Cmd(10.0f)};
    CHECK(!Detectors::AimSnap::FindSettledSnap(still, cfg).has_value());

    std::array<UserCmdView, 5> smallFlick = {Cmd(30.0f), Cmd(10.0f), Cmd(10.0f), Cmd(10.0f), Cmd(10.0f)};  // 20 deg
    CHECK(!Detectors::AimSnap::FindSettledSnap(smallFlick, cfg).has_value());
}

// --- B2: spin subtick unwrap ------------------------------------------------

TEST_CASE("SpinYawDelta unwraps the subtick sum, defeating the viewangle wrap alias")
{
    UserCmdView c = Cmd(2.0f);  // visible yaw barely moved (a full rotation aliased to ~0)
    c.SubtickMoveCount = 4;
    for (int i = 0; i < 4; ++i)
        c.SubtickMoves[i].YawDelta = 90.0f;  // true rotation this tick = 360 deg
    CHECK(Near(Detectors::Spinbot::SpinYawDelta(c, 2.0f), 360.0f));

    UserCmdView noSub = Cmd(2.0f);  // no subticks -> caller's normalized fallback
    CHECK(Near(Detectors::Spinbot::SpinYawDelta(noSub, 7.5f), 7.5f));
}

TEST_CASE("Spin detector arms above the velocity threshold and stays quiet below it")
{
    SpinSettings cfg;  // yawVelocityDegPerSec=720 -> 11.25 deg/tick at 64 tick
    auto feed = [&](float perTick, int ticks) {
        PlayerState s;
        bool fired = false;
        for (int i = 0; i < ticks; ++i)
            if (auto d = Detectors::Spinbot::OnCmd(cfg, s, perTick))
                fired = fired || std::strcmp(d->Detector, "spin.idle") == 0;
        return fired;
    };
    CHECK(feed(12.0f, 80));   // 768 deg/s -> arms
    CHECK(!feed(11.0f, 80));  // 704 deg/s -> below threshold
}

// --- B tuning: threshold reachability locks the config numbers ---------------

TEST_CASE("Retuned thresholds make minEvents the binding constraint at realistic spacing")
{
    SpinSettings spin;
    CHECK(ValueAtNthEvent(spin.minKillEvents, 30.0, spin.killEventScore, spin.decayPerSec) >= spin.banScore);
    CHECK(ValueAtNthEvent(spin.minKillEvents, 45.0, spin.killEventScore, spin.decayPerSec) < spin.banScore);
    CHECK(ValueAtNthEvent(spin.minKillEvents - 1, 30.0, spin.killEventScore, spin.decayPerSec) < spin.banScore);

    AimSnapSettings snap;
    CHECK(ValueAtNthEvent(snap.minEvents, 20.0, snap.confirmedHitScore, snap.decayPerSec) >= snap.banScore);
    CHECK(ValueAtNthEvent(snap.minEvents, 30.0, snap.confirmedHitScore, snap.decayPerSec) < snap.banScore);
    CHECK(ValueAtNthEvent(snap.minEvents - 1, 20.0, snap.confirmedHitScore, snap.decayPerSec) < snap.banScore);

    ShotAngleSettings shot;
    CHECK(ValueAtNthEvent(shot.minEvents, 20.0, shot.eventScore, shot.decayPerSec) >= shot.banScore);
    CHECK(ValueAtNthEvent(shot.minEvents, 30.0, shot.eventScore, shot.decayPerSec) < shot.banScore);

    NoFlashSettings flash;
    CHECK(ValueAtNthEvent(flash.minEvents, 20.0, flash.eventScore, flash.decayPerSec) >= flash.banScore);
    CHECK(ValueAtNthEvent(flash.minEvents, 30.0, flash.eventScore, flash.decayPerSec) < flash.banScore);
}
