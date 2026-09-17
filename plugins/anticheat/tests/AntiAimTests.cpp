#include "Detect/Rules/AntiAim.hpp"

#include <cmath>
#include <doctest/doctest.h>

using Anticheat::Rules::AntiAim;
using Anticheat::CmdSample;
using Anticheat::Finding;
using Anticheat::ShotView;

static constexpr int Slot = 0;
static constexpr double Now = 100.0;

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

static std::optional<Finding> Feed(AntiAim& rule, const CmdSample& cmd, int32_t serverTick, double now = Now,
                                   bool teleported = false)
{
    rule.OnCommand(Slot, cmd);
    return rule.OnSimulated(Slot, cmd.CmdNum, serverTick, true, teleported, now);
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
static int RunPattern(AntiAim& rule, int count, float (*yawAt)(int))
{
    for (int i = 1; i <= count; ++i)
        if (Feed(rule, Cmd(i, i, yawAt(i)), i))
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
    AntiAim beyond;
    Feed(beyond, Cmd(1, 1, 0.0f, 89.02f), 1);
    CHECK(beyond.Score(Slot) == doctest::Approx(2.0f));

    AntiAim within;
    Feed(within, Cmd(1, 1, 0.0f, 89.00f), 1);
    CHECK(within.Score(Slot) == doctest::Approx(0.0f));
}

TEST_CASE("A roll past 50.01 degrees scores and 50.00 does not")
{
    AntiAim beyond;
    Feed(beyond, Cmd(1, 1, 0.0f, 0.0f, 50.02f), 1);
    CHECK(beyond.Score(Slot) == doctest::Approx(2.0f));

    AntiAim within;
    Feed(within, Cmd(1, 1, 0.0f, 0.0f, 50.00f), 1);
    CHECK(within.Score(Slot) == doctest::Approx(0.0f));
}

TEST_CASE("A negative pitch past the limit scores just like a positive one")
{
    AntiAim rule;
    Feed(rule, Cmd(1, 1, 0.0f, -89.5f), 1);
    CHECK(rule.Score(Slot) == doctest::Approx(2.0f));
}

TEST_CASE("An inconsistent command scores whatever made it inconsistent")
{
    AntiAim rule;
    CmdSample cmd = Cmd(1, 1);
    cmd.AttackIndexInvalid = true;
    Feed(rule, cmd, 1);
    CHECK(rule.Score(Slot) == doctest::Approx(1.0f));
}

TEST_CASE("Base and history yaw mismatches only score every fourth command")
{
    AntiAim rule;
    for (int32_t i = 1; i <= 5; ++i)
        Feed(rule, MismatchCmd(i, i), i);
    // Commands 1 and 5 scored; 2, 3 and 4 fell inside the spacing.
    CHECK(rule.Score(Slot) == doctest::Approx(2.0f));
}

TEST_CASE("A mismatch under 120 degrees is not evidence at all")
{
    AntiAim rule;
    for (int32_t i = 1; i <= 5; ++i)
    {
        CmdSample cmd = MismatchCmd(i, i);
        cmd.MaxHistoryYawDelta = 119.0f;
        Feed(rule, cmd, i);
    }
    CHECK(rule.Score(Slot) == doctest::Approx(0.0f));
}

TEST_CASE("A command that started an attack is exempt from the mismatch rule")
{
    AntiAim rule;
    CmdSample cmd = MismatchCmd(1, 1);
    cmd.AttackStarted = true;
    Feed(rule, cmd, 1);
    CHECK(rule.Score(Slot) == doctest::Approx(0.0f));
}

TEST_CASE("The score decays at two points a second")
{
    AntiAim rule;
    for (int32_t i = 1; i <= 3; ++i)
        Feed(rule, Cmd(i, i, 0.0f, 89.5f), i);
    CHECK(rule.Score(Slot) == doctest::Approx(6.0f));

    Feed(rule, Cmd(4, 4), 4, Now + 1.0);
    CHECK(rule.Score(Slot) == doctest::Approx(4.0f));

    Feed(rule, Cmd(5, 5), 5, Now + 3.0);
    CHECK(rule.Score(Slot) == doctest::Approx(0.0f));
}

TEST_CASE("Nothing scores inside the five second spawn or teleport grace")
{
    AntiAim rule;
    Feed(rule, Cmd(1, 1, 0.0f, 89.5f), 1, Now, true);
    CHECK(rule.Score(Slot) == doctest::Approx(0.0f));

    Feed(rule, Cmd(2, 2, 0.0f, 89.5f), 2, Now, false);
    CHECK(rule.Score(Slot) == doctest::Approx(2.0f));
}

TEST_CASE("A one command excursion around a shot that returns to the surrounding angle scores")
{
    AntiAim rule;
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
    rule.OnWeaponFire(Slot, shot, Now);
    CHECK(rule.Score(Slot) == doctest::Approx(5.0f));
}

TEST_CASE("A shot excursion under thirty degrees is not an attack return")
{
    AntiAim rule;
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
    rule.OnWeaponFire(Slot, shot, Now);
    CHECK(rule.Score(Slot) == doctest::Approx(0.0f));
}

TEST_CASE("A sustained one direction spin fires after ten seconds of the slow tier")
{
    AntiAim rule;
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
    AntiAim rule;
    // 40 degrees a tick is 2560 degrees a second: the fast tier.
    const int fired = RunPattern(rule, 400, FastSpin);
    REQUIRE(fired > 0);
    CHECK(fired >= 200);
    CHECK(fired <= 215);
}

TEST_CASE("A spin keeps accruing when the server simulates commands in uneven batches")
{
    AntiAim rule;
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
    AntiAim rule;
    CHECK(RunPattern(rule, 1200, ReversingYaw) == -1);
}

TEST_CASE("A spin broken by more than a second of missing commands loses its progress")
{
    AntiAim rule;
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
    AntiAim rule;
    const int fired = RunPattern(rule, 900, TwoWayJitter);
    REQUIRE(fired > 0);
    // Period 2 needs eight commands of history, so 320 ticks later is command 327.
    CHECK(fired >= 320);
    CHECK(fired <= 345);
}

TEST_CASE("A two way yaw pattern spanning under ten degrees is not jitter")
{
    AntiAim rule;
    CHECK(RunPattern(rule, 900, ReversingYaw) == -1);
}

TEST_CASE("A slot change drops the slot's accumulated anti aim score")
{
    AntiAim rule;
    Feed(rule, Cmd(1, 1, 0.0f, 89.5f), 1);
    CHECK(rule.Score(Slot) == doctest::Approx(2.0f));
    rule.OnSlotChanged(Slot);
    CHECK(rule.Score(Slot) == doctest::Approx(0.0f));
}

TEST_CASE("A player who is no longer eligible has their history and score dropped")
{
    AntiAim rule;
    Feed(rule, Cmd(1, 1, 0.0f, 89.5f), 1);
    rule.OnCommand(Slot, Cmd(2, 2));
    rule.OnSimulated(Slot, 2, 2, false, false, Now);
    // The next invalid command starts a fresh history rather than continuing an episode.
    Feed(rule, Cmd(3, 3, 0.0f, 89.5f), 3);
    CHECK(rule.Score(Slot) == doctest::Approx(4.0f));
}
