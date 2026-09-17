#include "Detect/Rules/AntiAim.hpp"

#include <cmath>
#include <doctest/doctest.h>

using Anticheat::Rules::AntiAim;
using Anticheat::CmdSample;
using Anticheat::DetectionKind;
using Anticheat::Finding;
using Anticheat::ShotView;
using Anticheat::Suspicion;

static constexpr int Slot = 0;
static constexpr double Now = 100.0;

/** The rule and the score it feeds, since a finding now comes out of the score. */
struct AntiAimHarness
{
    AntiAimHarness() = default;

    /** Evidence points this rule has on the slot, as of @p now. */
    float Points(double now = Now) const { return Scores.Value(Slot, DetectionKind::AntiAim, now) * 100.0f; }

    Suspicion Scores;
    AntiAim Rule{Scores};
};

static CmdSample Cmd(int32_t num, int32_t clientTick, float yaw = 0.0f, float pitch = 0.0f, float roll = 0.0f)
{
    CmdSample cmd;
    cmd.CmdNum = num;
    cmd.ClientTick = clientTick;
    cmd.ViewYaw = yaw;
    cmd.ViewPitch = pitch;
    cmd.ViewRoll = roll;
    return cmd;
}

static std::optional<Finding> Feed(AntiAimHarness& h, const CmdSample& cmd, int32_t serverTick, double now = Now,
                                   bool teleported = false)
{
    h.Rule.OnCommand(Slot, cmd);
    return h.Rule.OnSimulated(Slot, cmd.CmdNum, serverTick, true, teleported, now);
}

/** A command whose base view angle disagrees with the angles it claims it fired along. */
static CmdSample MismatchCmd(int32_t num, int32_t tick)
{
    CmdSample cmd = Cmd(num, tick);
    cmd.HasHistoryAngles = true;
    cmd.MaxHistoryYawDelta = 130.0f;
    return cmd;
}

/** Feeds @p count commands whose yaw follows @p yawAt, and returns the command number that fired. */
static int RunPattern(AntiAimHarness& h, int count, float (*yawAt)(int))
{
    for (int i = 1; i <= count; ++i)
        if (Feed(h, Cmd(i, i, yawAt(i)), i))
            return i;
    return -1;
}

static float SteadySpin(int i)
{
    return std::fmod(6.0f * static_cast<float>(i), 360.0f);
}

static float ReversingYaw(int i)
{
    return i % 2 == 0 ? 6.0f : 0.0f;
}

static float TwoWayJitter(int i)
{
    return i % 2 == 0 ? 20.0f : 0.0f;
}

TEST_CASE("A pitch past 89.01 degrees scores and 89.00 does not")
{
    AntiAimHarness beyond;
    Feed(beyond, Cmd(1, 1, 0.0f, 89.02f), 1);
    CHECK(beyond.Points() == doctest::Approx(2.0f));

    AntiAimHarness within;
    Feed(within, Cmd(1, 1, 0.0f, 89.00f), 1);
    CHECK(within.Points() == doctest::Approx(0.0f));
}

TEST_CASE("A roll past 50.01 degrees scores and 50.00 does not")
{
    AntiAimHarness beyond;
    Feed(beyond, Cmd(1, 1, 0.0f, 0.0f, 50.02f), 1);
    CHECK(beyond.Points() == doctest::Approx(2.0f));

    AntiAimHarness within;
    Feed(within, Cmd(1, 1, 0.0f, 0.0f, 50.00f), 1);
    CHECK(within.Points() == doctest::Approx(0.0f));
}

TEST_CASE("A negative pitch past the limit scores just like a positive one")
{
    AntiAimHarness rule;
    Feed(rule, Cmd(1, 1, 0.0f, -89.5f), 1);
    CHECK(rule.Points() == doctest::Approx(2.0f));
}

TEST_CASE("An inconsistent command scores whatever made it inconsistent")
{
    AntiAimHarness rule;
    CmdSample cmd = Cmd(1, 1);
    cmd.AttackIndexInvalid = true;
    Feed(rule, cmd, 1);
    CHECK(rule.Points() == doctest::Approx(1.0f));
}

TEST_CASE("Base and history yaw mismatches only score every fourth command")
{
    AntiAimHarness rule;
    for (int32_t i = 1; i <= 5; ++i)
        Feed(rule, MismatchCmd(i, i), i);
    // Commands 1 and 5 scored; 2, 3 and 4 fell inside the spacing. This rule is worth 0.4 a
    // command, where it used to be worth 1 and then decay two and a half times faster.
    CHECK(rule.Points() == doctest::Approx(0.8f));
}

TEST_CASE("A mismatch under 120 degrees is not evidence at all")
{
    AntiAimHarness rule;
    for (int32_t i = 1; i <= 5; ++i)
    {
        CmdSample cmd = MismatchCmd(i, i);
        cmd.MaxHistoryYawDelta = 119.0f;
        Feed(rule, cmd, i);
    }
    CHECK(rule.Points() == doctest::Approx(0.0f));
}

TEST_CASE("A command that started an attack is exempt from the mismatch rule")
{
    AntiAimHarness rule;
    CmdSample cmd = MismatchCmd(1, 1);
    cmd.AttackStarted = true;
    Feed(rule, cmd, 1);
    CHECK(rule.Points() == doctest::Approx(0.0f));
}

TEST_CASE("Anti-aim evidence fades on a half-life of thirty seconds")
{
    AntiAimHarness rule;
    for (int32_t i = 1; i <= 3; ++i)
        Feed(rule, Cmd(i, i, 0.0f, 89.5f), i);
    CHECK(rule.Points() == doctest::Approx(6.0f));

    // This rule sees evidence at command rate, so it fades far faster than the shot-driven ones.
    CHECK(rule.Points(Now + 30.0) == doctest::Approx(3.0f));
    CHECK(rule.Points(Now + 60.0) == doctest::Approx(1.5f));
}

TEST_CASE("Nothing scores inside the five second spawn or teleport grace")
{
    AntiAimHarness rule;
    Feed(rule, Cmd(1, 1, 0.0f, 89.5f), 1, Now, true);
    CHECK(rule.Points() == doctest::Approx(0.0f));

    Feed(rule, Cmd(2, 2, 0.0f, 89.5f), 2, Now, false);
    CHECK(rule.Points() == doctest::Approx(2.0f));
}

TEST_CASE("A one command excursion around a shot that returns to the surrounding angle scores")
{
    AntiAimHarness rule;
    CmdSample shotCmd = Cmd(2, 2, 45.0f);
    shotCmd.AttackStarted = true;
    Feed(rule, Cmd(1, 1, 0.0f), 1);
    Feed(rule, shotCmd, 2);
    Feed(rule, Cmd(3, 3, 0.2f), 3);

    ShotView shot;
    shot.Slot = Slot;
    shot.CmdNum = 2;
    shot.ServerTick = 2;
    shot.FireTick = 2;
    rule.Rule.OnWeaponFire(Slot, shot, Now);
    CHECK(rule.Points() == doctest::Approx(5.0f));
}

TEST_CASE("A shot excursion under thirty degrees is not an attack return")
{
    AntiAimHarness rule;
    CmdSample shotCmd = Cmd(2, 2, 20.0f);
    shotCmd.AttackStarted = true;
    Feed(rule, Cmd(1, 1, 0.0f), 1);
    Feed(rule, shotCmd, 2);
    Feed(rule, Cmd(3, 3, 0.2f), 3);

    ShotView shot;
    shot.Slot = Slot;
    shot.CmdNum = 2;
    shot.ServerTick = 2;
    shot.FireTick = 2;
    rule.Rule.OnWeaponFire(Slot, shot, Now);
    CHECK(rule.Points() == doctest::Approx(0.0f));
}

TEST_CASE("A sustained one direction spin fires after ten seconds of the slow tier")
{
    AntiAimHarness rule;
    // 6 degrees a tick is 384 degrees a second: above the 320 tier, below the 1000 one.
    const int fired = RunPattern(rule, 1200, SteadySpin);
    REQUIRE(fired > 0);
    // Progress starts once sixteen consecutive commands exist, so 640 ticks later is command 655.
    CHECK(fired >= 650);
    CHECK(fired <= 665);
}

static float FastSpin(int i)
{
    return std::fmod(40.0f * static_cast<float>(i), 360.0f);
}

TEST_CASE("A fast spin fires after three seconds")
{
    AntiAimHarness rule;
    // 40 degrees a tick is 2560 degrees a second: the fast tier.
    const int fired = RunPattern(rule, 400, FastSpin);
    REQUIRE(fired > 0);
    CHECK(fired >= 200);
    CHECK(fired <= 215);
}

TEST_CASE("A spin keeps accruing when the server simulates commands in uneven batches")
{
    AntiAimHarness rule;
    // Two commands land on one server tick, none on the next: the client sequence is unbroken.
    int fired = -1;
    for (int i = 1; i <= 1200 && fired < 0; ++i)
        if (Feed(rule, Cmd(i, i, SteadySpin(i)), (i / 2) * 2))
            fired = i;
    REQUIRE(fired > 0);
    CHECK(fired <= 665);
}

TEST_CASE("A yaw that reverses every tick never reaches the spin direction consistency")
{
    AntiAimHarness rule;
    CHECK(RunPattern(rule, 1200, ReversingYaw) == -1);
}

TEST_CASE("A spin broken by more than a second of missing commands loses its progress")
{
    AntiAimHarness rule;
    for (int32_t i = 1; i <= 400; ++i)
        REQUIRE_FALSE(Feed(rule, Cmd(i, i, SteadySpin(i)), i).has_value());

    // Two seconds of commands the server never received.
    std::optional<Finding> finding;
    for (int32_t i = 529; i <= 928 && !finding; ++i)
        finding = Feed(rule, Cmd(i, i, SteadySpin(i)), i);
    // Uninterrupted, 800 commands would have fired; the reset means these 400 are not enough.
    CHECK_FALSE(finding.has_value());
}

TEST_CASE("An exact two way yaw pattern fires after five seconds of jitter")
{
    AntiAimHarness rule;
    const int fired = RunPattern(rule, 900, TwoWayJitter);
    REQUIRE(fired > 0);
    // Period 2 needs eight commands of history, so 320 ticks later is command 327.
    CHECK(fired >= 320);
    CHECK(fired <= 345);
}

TEST_CASE("A two way yaw pattern spanning under ten degrees is not jitter")
{
    AntiAimHarness rule;
    CHECK(RunPattern(rule, 900, ReversingYaw) == -1);
}

TEST_CASE("A slot change drops the slot's accumulated anti aim score")
{
    AntiAimHarness rule;
    Feed(rule, Cmd(1, 1, 0.0f, 89.5f), 1);
    CHECK(rule.Points() == doctest::Approx(2.0f));
    rule.Rule.OnSlotChanged(Slot);
    rule.Scores.OnSlotChanged(Slot);
    CHECK(rule.Points() == doctest::Approx(0.0f));
}

TEST_CASE("A player who is no longer eligible starts a fresh episode when they return")
{
    AntiAimHarness rule;
    Feed(rule, Cmd(1, 1, 0.0f, 89.5f), 1);
    rule.Rule.OnCommand(Slot, Cmd(2, 2));
    rule.Rule.OnSimulated(Slot, 2, 2, false, false, Now);
    // The next invalid command starts a fresh history rather than continuing an episode.
    Feed(rule, Cmd(3, 3, 0.0f, 89.5f), 3);
    CHECK(rule.Points() == doctest::Approx(4.0f));
}
