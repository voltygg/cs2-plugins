#include "Detect/ShotHistory.hpp"

#include <array>
#include <doctest/doctest.h>

using Anticheat::AimAngles;
using Anticheat::CmdSample;
using Anticheat::MaxSlots;
using Anticheat::PositionSample;
using Anticheat::ShotHistory;
using Anticheat::ShotView;
using Anticheat::TeamCT;
using Anticheat::TeamT;

static constexpr int Shooter = 0;
static constexpr int Victim = 1;

static CmdSample Cmd(int32_t num, int32_t clientTick, float yaw = 0.0f)
{
    CmdSample cmd;
    cmd.CmdNum = num;
    cmd.ClientTick = clientTick;
    cmd.ViewYaw = yaw;
    cmd.AttackStarted = true;
    cmd.AttackAngles = AimAngles{0.0f, yaw};
    return cmd;
}

static std::array<PositionSample, MaxSlots> Players()
{
    std::array<PositionSample, MaxSlots> players{};
    players[Shooter] = {
        .Origin = {0.0f, 0.0f, 0.0f}, .EyePos = {0.0f, 0.0f, 64.0f}, .Team = TeamT, .Valid = true, .Alive = true};
    players[Victim] = {
        .Origin = {500.0f, 0.0f, 0.0f}, .EyePos = {500.0f, 0.0f, 64.0f}, .Team = TeamCT, .Valid = true, .Alive = true};
    return players;
}

/** One command, simulated at @p serverTick, ready to be matched by a fire event. */
static void FeedSimulated(ShotHistory& history, int32_t cmdNum, int32_t serverTick, float yaw = 0.0f)
{
    history.OnCommand(Shooter, Cmd(cmdNum, serverTick, yaw));
    history.OnSimulated(Shooter, cmdNum, serverTick, {0.0f, 0.0f, 64.0f}, false);
}

TEST_CASE("A command whose attack angles were capped away never enters the ring")
{
    ShotHistory history;
    CmdSample cmd = Cmd(1, 1);
    cmd.AttackAngles.reset();  // the index pointed past the input-history cap
    history.OnCommand(Shooter, cmd);
    CHECK(history.CommandCount(Shooter) == 0);

    // Clamping such an index back into range would read another shot's angles.
    history.OnCommand(Shooter, Cmd(2, 2));
    CHECK(history.CommandCount(Shooter) == 1);
}

TEST_CASE("A repeated command number is dropped rather than stored twice")
{
    ShotHistory history;
    history.OnCommand(Shooter, Cmd(7, 1));
    history.OnCommand(Shooter, Cmd(7, 2));
    CHECK(history.CommandCount(Shooter) == 1);
    history.OnCommand(Shooter, Cmd(8, 3));
    CHECK(history.CommandCount(Shooter) == 2);
}

TEST_CASE("The command ring holds at most 32 commands")
{
    ShotHistory history;
    for (int32_t i = 0; i < 100; ++i)
        history.OnCommand(Shooter, Cmd(i, i));
    CHECK(history.CommandCount(Shooter) == 32);
}

TEST_CASE("A fire matches a command in the same tick and one tick later but not two")
{
    ShotHistory history;
    history.CaptureFrame(10, Players());

    FeedSimulated(history, 1, 10);
    CHECK(history.OnWeaponFire(Shooter, "weapon_ak47", 10, {}, false) != nullptr);

    ShotHistory next;
    FeedSimulated(next, 1, 10);
    CHECK(next.OnWeaponFire(Shooter, "ak47", 11, {}, false) != nullptr);

    ShotHistory late;
    FeedSimulated(late, 1, 10);
    CHECK(late.OnWeaponFire(Shooter, "ak47", 12, {}, false) == nullptr);
}

TEST_CASE("A fire from a non ballistic weapon is ignored entirely")
{
    ShotHistory history;
    FeedSimulated(history, 1, 10);
    CHECK(history.OnWeaponFire(Shooter, "weapon_hegrenade", 10, {}, false) == nullptr);
    CHECK(history.Shots(Shooter).empty());
    // The command was not burned, so a real fire can still claim it.
    CHECK(history.OnWeaponFire(Shooter, "ak47", 10, {}, false) != nullptr);
}

TEST_CASE("Two candidate commands in the window produce no shot and both are consumed")
{
    ShotHistory history;
    FeedSimulated(history, 1, 10);
    FeedSimulated(history, 2, 11);

    CHECK(history.OnWeaponFire(Shooter, "ak47", 11, {}, false) == nullptr);
    CHECK(history.Shots(Shooter).empty());
    // Both were burned: a second fire in the same window cannot arbitrarily pick one of them.
    CHECK(history.OnWeaponFire(Shooter, "ak47", 11, {}, false) == nullptr);
}

TEST_CASE("Impact hurt and death attach to the shot exactly once each")
{
    ShotHistory history;
    history.CaptureFrame(10, Players());
    FeedSimulated(history, 1, 10);
    REQUIRE(history.OnWeaponFire(Shooter, "ak47", 10, {0.0f, 0.0f}, true) != nullptr);

    ShotView* impact = history.OnBulletImpact(Shooter, {500.0f, 0.0f, 60.0f}, 10);
    REQUIRE(impact != nullptr);
    CHECK(impact->ImpactSeen);
    CHECK(history.OnBulletImpact(Shooter, {500.0f, 0.0f, 60.0f}, 10) == nullptr);

    ShotView* hurt = history.OnPlayerHurt(Shooter, Victim, true, 10);
    REQUIRE(hurt != nullptr);
    CHECK(hurt->HurtSeen);
    CHECK(hurt->Headshot);
    CHECK(hurt->VictimSlot == Victim);
    CHECK(history.OnPlayerHurt(Shooter, Victim, true, 10) == nullptr);

    ShotView* death = history.OnPlayerDeath(Shooter, Victim, "weapon_ak47", true, 10);
    REQUIRE(death != nullptr);
    CHECK(death->Wallbang);
    CHECK(history.OnPlayerDeath(Shooter, Victim, "ak47", true, 10) == nullptr);
}

TEST_CASE("A death naming a different weapon does not match the shot")
{
    ShotHistory history;
    FeedSimulated(history, 1, 10);
    REQUIRE(history.OnWeaponFire(Shooter, "ak47", 10, {}, false) != nullptr);
    CHECK(history.OnPlayerDeath(Shooter, Victim, "weapon_awp", false, 10) == nullptr);
}

TEST_CASE("Self inflicted damage never matches a shot")
{
    ShotHistory history;
    FeedSimulated(history, 1, 10);
    REQUIRE(history.OnWeaponFire(Shooter, "ak47", 10, {}, false) != nullptr);
    CHECK(history.OnPlayerHurt(Shooter, Shooter, false, 10) == nullptr);
}

TEST_CASE("Shots survive two ticks and are pruned on the third")
{
    ShotHistory history;
    FeedSimulated(history, 1, 10);
    REQUIRE(history.OnWeaponFire(Shooter, "ak47", 10, {}, false) != nullptr);

    history.Prune(12);
    CHECK(history.Shots(Shooter).size() == 1);
    history.Prune(13);
    CHECK(history.Shots(Shooter).empty());
}

TEST_CASE("A slot change drops the slot's pending shots and commands")
{
    ShotHistory history;
    FeedSimulated(history, 1, 10);
    REQUIRE(history.OnWeaponFire(Shooter, "ak47", 10, {}, false) != nullptr);

    history.ClearSlot(Shooter);
    CHECK(history.Shots(Shooter).empty());
    CHECK(history.CommandCount(Shooter) == 0);
}

TEST_CASE("Reset clears frames and every slot's history")
{
    ShotHistory history;
    history.CaptureFrame(10, Players());
    FeedSimulated(history, 1, 10);
    history.Reset();
    CHECK(history.FrameCount() == 0);
    CHECK(history.CommandCount(Shooter) == 0);
    CHECK(history.FindFrame(10) == nullptr);
}

TEST_CASE("A truncated user id resolves only when both the slot and its shot are unambiguous")
{
    std::array<int32_t, MaxSlots> userIds{};
    userIds.fill(-1);
    userIds[Shooter] = 0x105;  // low byte 0x05
    userIds[Victim] = 0x205;   // the same low byte, a different player

    ShotHistory history;
    FeedSimulated(history, 1, 10);
    REQUIRE(history.OnWeaponFire(Shooter, "ak47", 10, {}, false) != nullptr);
    CHECK(history.ResolveImpactShooter(0x05, 10, userIds) == Shooter);

    // Once the other player with the same low byte also has an in-window shot, it is ambiguous.
    history.OnCommand(Victim, Cmd(1, 10));
    history.OnSimulated(Victim, 1, 10, {500.0f, 0.0f, 64.0f}, false);
    REQUIRE(history.OnWeaponFire(Victim, "ak47", 10, {}, false) != nullptr);
    CHECK(history.ResolveImpactShooter(0x05, 10, userIds) == -1);
}

TEST_CASE("A truncated user id resolves to nothing when the matching slot has several shots")
{
    std::array<int32_t, MaxSlots> userIds{};
    userIds.fill(-1);
    userIds[Shooter] = 0x05;

    ShotHistory history;
    FeedSimulated(history, 1, 10);
    REQUIRE(history.OnWeaponFire(Shooter, "ak47", 10, {}, false) != nullptr);
    FeedSimulated(history, 2, 11);
    REQUIRE(history.OnWeaponFire(Shooter, "ak47", 11, {}, false) != nullptr);
    CHECK(history.ResolveImpactShooter(0x05, 11, userIds) == -1);
}

TEST_CASE("The position history holds 128 frames and replaces a repeated newest tick")
{
    ShotHistory history;
    for (int32_t tick = 0; tick < 200; ++tick)
        history.CaptureFrame(tick, Players());
    CHECK(history.FrameCount() == 128);
    CHECK(history.FindFrame(71) == nullptr);
    CHECK(history.FindFrame(72) != nullptr);
    CHECK(history.FindFrame(199) != nullptr);

    auto players = Players();
    players[Victim].Origin = {900.0f, 0.0f, 0.0f};
    history.CaptureFrame(199, players);
    CHECK(history.FrameCount() == 128);
    REQUIRE(history.FindPosition(199, Victim).has_value());
    CHECK(history.FindPosition(199, Victim)->Origin.X == doctest::Approx(900.0f));
}

TEST_CASE("FindPosition returns nothing for a slot that was not in the frame")
{
    ShotHistory history;
    history.CaptureFrame(5, Players());
    CHECK(history.FindPosition(5, Shooter).has_value());
    CHECK(!history.FindPosition(5, 9).has_value());
    CHECK(!history.FindPosition(4, Shooter).has_value());
}

TEST_CASE("AreOpponents requires both teams to be playing and honours free for all")
{
    CHECK(ShotHistory::AreOpponents(TeamT, TeamCT, false));
    CHECK_FALSE(ShotHistory::AreOpponents(TeamT, TeamT, false));
    CHECK(ShotHistory::AreOpponents(TeamT, TeamT, true));
    CHECK_FALSE(ShotHistory::AreOpponents(1, TeamCT, true));  // spectator
    CHECK_FALSE(ShotHistory::AreOpponents(0, 0, true));       // unassigned

    ShotHistory history;
    CHECK_FALSE(history.AreOpponents(TeamT, TeamT));
    history.SetTeammatesAreEnemies(true);
    CHECK(history.AreOpponents(TeamT, TeamT));
}
