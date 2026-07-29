#include "Correlation/ShotCorrelatorCore.hpp"

#include <array>
#include <doctest/doctest.h>

using namespace Anticheat;

namespace
{
constexpr int Shooter = 0;
constexpr int Victim = 1;

CmdSample Cmd(int32_t num, int32_t clientTick, float yaw = 0.0f)
{
    CmdSample cmd;
    cmd.CmdNum = num;
    cmd.ClientTick = clientTick;
    cmd.ViewYaw = yaw;
    cmd.AttackStarted = true;
    cmd.AttackAngles = AimAngles{0.0f, yaw};
    return cmd;
}

std::array<PositionSample, MaxSlots> Players()
{
    std::array<PositionSample, MaxSlots> players{};
    players[Shooter] = {
        .Origin = {0.0f, 0.0f, 0.0f}, .EyePos = {0.0f, 0.0f, 64.0f}, .Team = TeamT, .Valid = true, .Alive = true};
    players[Victim] = {
        .Origin = {500.0f, 0.0f, 0.0f}, .EyePos = {500.0f, 0.0f, 64.0f}, .Team = TeamCT, .Valid = true, .Alive = true};
    return players;
}

/** One command, simulated at @p serverTick, ready to be matched by a fire event. */
void FeedSimulated(ShotCorrelatorCore& core, int32_t cmdNum, int32_t serverTick, float yaw = 0.0f)
{
    core.OnCommand(Shooter, Cmd(cmdNum, serverTick, yaw));
    core.OnSimulated(Shooter, cmdNum, serverTick, {0.0f, 0.0f, 64.0f}, false);
}
}  // namespace

TEST_CASE("A command whose attack angles were capped away never enters the ring")
{
    ShotCorrelatorCore core;
    CmdSample cmd = Cmd(1, 1);
    cmd.AttackAngles.reset();  // the index pointed past the input-history cap
    core.OnCommand(Shooter, cmd);
    CHECK(core.CommandCount(Shooter) == 0);

    // Clamping such an index back into range would read another shot's angles.
    core.OnCommand(Shooter, Cmd(2, 2));
    CHECK(core.CommandCount(Shooter) == 1);
}

TEST_CASE("A repeated command number is dropped rather than stored twice")
{
    ShotCorrelatorCore core;
    core.OnCommand(Shooter, Cmd(7, 1));
    core.OnCommand(Shooter, Cmd(7, 2));
    CHECK(core.CommandCount(Shooter) == 1);
    core.OnCommand(Shooter, Cmd(8, 3));
    CHECK(core.CommandCount(Shooter) == 2);
}

TEST_CASE("The command ring holds at most 32 commands")
{
    ShotCorrelatorCore core;
    for (int32_t i = 0; i < 100; ++i)
        core.OnCommand(Shooter, Cmd(i, i));
    CHECK(core.CommandCount(Shooter) == 32);
}

TEST_CASE("A fire matches a command in the same tick and one tick later but not two")
{
    ShotCorrelatorCore core;
    core.CaptureFrame(10, Players());

    FeedSimulated(core, 1, 10);
    CHECK(core.OnWeaponFire(Shooter, "weapon_ak47", 10, {}, false) != nullptr);

    ShotCorrelatorCore next;
    FeedSimulated(next, 1, 10);
    CHECK(next.OnWeaponFire(Shooter, "ak47", 11, {}, false) != nullptr);

    ShotCorrelatorCore late;
    FeedSimulated(late, 1, 10);
    CHECK(late.OnWeaponFire(Shooter, "ak47", 12, {}, false) == nullptr);
}

TEST_CASE("A fire from a non ballistic weapon is ignored entirely")
{
    ShotCorrelatorCore core;
    FeedSimulated(core, 1, 10);
    CHECK(core.OnWeaponFire(Shooter, "weapon_hegrenade", 10, {}, false) == nullptr);
    CHECK(core.Shots(Shooter).empty());
    // The command was not burned, so a real fire can still claim it.
    CHECK(core.OnWeaponFire(Shooter, "ak47", 10, {}, false) != nullptr);
}

TEST_CASE("Two candidate commands in the window produce no shot and both are consumed")
{
    ShotCorrelatorCore core;
    FeedSimulated(core, 1, 10);
    FeedSimulated(core, 2, 11);

    CHECK(core.OnWeaponFire(Shooter, "ak47", 11, {}, false) == nullptr);
    CHECK(core.Shots(Shooter).empty());
    // Both were burned: a second fire in the same window cannot arbitrarily pick one of them.
    CHECK(core.OnWeaponFire(Shooter, "ak47", 11, {}, false) == nullptr);
}

TEST_CASE("Impact hurt and death attach to the shot exactly once each")
{
    ShotCorrelatorCore core;
    core.CaptureFrame(10, Players());
    FeedSimulated(core, 1, 10);
    REQUIRE(core.OnWeaponFire(Shooter, "ak47", 10, {0.0f, 0.0f}, true) != nullptr);

    ShotView* impact = core.OnBulletImpact(Shooter, {500.0f, 0.0f, 60.0f}, 10);
    REQUIRE(impact != nullptr);
    CHECK(impact->ImpactSeen);
    CHECK(core.OnBulletImpact(Shooter, {500.0f, 0.0f, 60.0f}, 10) == nullptr);

    ShotView* hurt = core.OnPlayerHurt(Shooter, Victim, true, 10);
    REQUIRE(hurt != nullptr);
    CHECK(hurt->HurtSeen);
    CHECK(hurt->Headshot);
    CHECK(hurt->VictimSlot == Victim);
    CHECK(core.OnPlayerHurt(Shooter, Victim, true, 10) == nullptr);

    ShotView* death = core.OnPlayerDeath(Shooter, Victim, "weapon_ak47", true, 10);
    REQUIRE(death != nullptr);
    CHECK(death->Wallbang);
    CHECK(core.OnPlayerDeath(Shooter, Victim, "ak47", true, 10) == nullptr);
}

TEST_CASE("A death naming a different weapon does not match the shot")
{
    ShotCorrelatorCore core;
    FeedSimulated(core, 1, 10);
    REQUIRE(core.OnWeaponFire(Shooter, "ak47", 10, {}, false) != nullptr);
    CHECK(core.OnPlayerDeath(Shooter, Victim, "weapon_awp", false, 10) == nullptr);
}

TEST_CASE("Self inflicted damage never matches a shot")
{
    ShotCorrelatorCore core;
    FeedSimulated(core, 1, 10);
    REQUIRE(core.OnWeaponFire(Shooter, "ak47", 10, {}, false) != nullptr);
    CHECK(core.OnPlayerHurt(Shooter, Shooter, false, 10) == nullptr);
}

TEST_CASE("Shots survive two ticks and are pruned on the third")
{
    ShotCorrelatorCore core;
    FeedSimulated(core, 1, 10);
    REQUIRE(core.OnWeaponFire(Shooter, "ak47", 10, {}, false) != nullptr);

    core.Prune(12);
    CHECK(core.Shots(Shooter).size() == 1);
    core.Prune(13);
    CHECK(core.Shots(Shooter).empty());
}

TEST_CASE("A generation bump invalidates the slot's pending shots and commands")
{
    ShotCorrelatorCore core;
    FeedSimulated(core, 1, 10);
    REQUIRE(core.OnWeaponFire(Shooter, "ak47", 10, {}, false) != nullptr);
    const uint32_t before = core.Generation(Shooter);

    core.OnSlotChanged(Shooter);
    CHECK(core.Generation(Shooter) == before + 1);
    CHECK(core.Shots(Shooter).empty());
    CHECK(core.CommandCount(Shooter) == 0);
}

TEST_CASE("Reset clears frames and every slot's history")
{
    ShotCorrelatorCore core;
    core.CaptureFrame(10, Players());
    FeedSimulated(core, 1, 10);
    core.Reset();
    CHECK(core.FrameCount() == 0);
    CHECK(core.CommandCount(Shooter) == 0);
    CHECK(core.FindFrame(10) == nullptr);
}

TEST_CASE("A truncated user id resolves only when both the slot and its shot are unambiguous")
{
    std::array<int32_t, MaxSlots> userIds{};
    userIds.fill(-1);
    userIds[Shooter] = 0x105;  // low byte 0x05
    userIds[Victim] = 0x205;   // the same low byte, a different player

    ShotCorrelatorCore core;
    FeedSimulated(core, 1, 10);
    REQUIRE(core.OnWeaponFire(Shooter, "ak47", 10, {}, false) != nullptr);
    CHECK(core.ResolveImpactShooter(0x05, 10, userIds) == Shooter);

    // Once the other player with the same low byte also has an in-window shot, it is ambiguous.
    core.OnCommand(Victim, Cmd(1, 10));
    core.OnSimulated(Victim, 1, 10, {500.0f, 0.0f, 64.0f}, false);
    REQUIRE(core.OnWeaponFire(Victim, "ak47", 10, {}, false) != nullptr);
    CHECK(core.ResolveImpactShooter(0x05, 10, userIds) == -1);
}

TEST_CASE("A truncated user id resolves to nothing when the matching slot has several shots")
{
    std::array<int32_t, MaxSlots> userIds{};
    userIds.fill(-1);
    userIds[Shooter] = 0x05;

    ShotCorrelatorCore core;
    FeedSimulated(core, 1, 10);
    REQUIRE(core.OnWeaponFire(Shooter, "ak47", 10, {}, false) != nullptr);
    FeedSimulated(core, 2, 11);
    REQUIRE(core.OnWeaponFire(Shooter, "ak47", 11, {}, false) != nullptr);
    CHECK(core.ResolveImpactShooter(0x05, 11, userIds) == -1);
}

TEST_CASE("The position history holds 128 frames and replaces a repeated newest tick")
{
    ShotCorrelatorCore core;
    for (int32_t tick = 0; tick < 200; ++tick)
        core.CaptureFrame(tick, Players());
    CHECK(core.FrameCount() == 128);
    CHECK(core.FindFrame(71) == nullptr);
    CHECK(core.FindFrame(72) != nullptr);
    CHECK(core.FindFrame(199) != nullptr);

    auto players = Players();
    players[Victim].Origin = {900.0f, 0.0f, 0.0f};
    core.CaptureFrame(199, players);
    CHECK(core.FrameCount() == 128);
    REQUIRE(core.FindPosition(199, Victim) != nullptr);
    CHECK(core.FindPosition(199, Victim)->Origin.X == doctest::Approx(900.0f));
}

TEST_CASE("FindPosition returns nothing for a slot that was not in the frame")
{
    ShotCorrelatorCore core;
    core.CaptureFrame(5, Players());
    CHECK(core.FindPosition(5, Shooter) != nullptr);
    CHECK(core.FindPosition(5, 9) == nullptr);
    CHECK(core.FindPosition(4, Shooter) == nullptr);
}

TEST_CASE("AreOpponents requires both teams to be playing and honours free for all")
{
    CHECK(ShotCorrelatorCore::AreOpponents(TeamT, TeamCT, false));
    CHECK_FALSE(ShotCorrelatorCore::AreOpponents(TeamT, TeamT, false));
    CHECK(ShotCorrelatorCore::AreOpponents(TeamT, TeamT, true));
    CHECK_FALSE(ShotCorrelatorCore::AreOpponents(1, TeamCT, true));  // spectator
    CHECK_FALSE(ShotCorrelatorCore::AreOpponents(0, 0, true));       // unassigned

    ShotCorrelatorCore core;
    CHECK_FALSE(core.AreOpponents(TeamT, TeamT));
    core.SetTeammatesAreEnemies(true);
    CHECK(core.AreOpponents(TeamT, TeamT));
}
