#include "Detectors/AimbotCore.hpp"

#include <array>
#include <doctest/doctest.h>

using namespace Anticheat;

namespace
{
constexpr int Attacker = 0;
constexpr int Victim = 1;
constexpr double Now = 1000.0;
constexpr Vec3 Eye{0.0f, 0.0f, 64.0f};

std::array<PositionSample, MaxSlots> Frame(float victimX = 500.0f, bool teleported = false)
{
    std::array<PositionSample, MaxSlots> players{};
    players[Attacker] = {.Origin = {0.0f, 0.0f, 0.0f}, .EyePos = Eye, .Team = TeamT, .Valid = true, .Alive = true};
    players[Victim] = {.Origin = {victimX, 0.0f, 0.0f},
                       .EyePos = {victimX, 0.0f, 64.0f},
                       .Team = TeamCT,
                       .Valid = true,
                       .Alive = true,
                       .Teleported = teleported};
    return players;
}

CmdSample AimCmd(int32_t num, int32_t clientTick, float yaw)
{
    CmdSample cmd;
    cmd.CmdNum = num;
    cmd.ClientTick = clientTick;
    cmd.ViewYaw = yaw;
    cmd.AttackStarted = true;
    cmd.AttackAngles = AimAngles{0.0f, yaw};
    return cmd;
}

/**
 * One damaging shot preceded by one adjacent command. With the eye level with the victim's tallest
 * body point, a level aim of y degrees is off target by exactly y, so the yaws below *are* the
 * before/after errors the convergence rule compares.
 */
struct Incident
{
    int32_t Base = 100;
    int32_t Tick = 100;
    float YawOlder = 12.0f;
    float YawShot = 1.9f;
    float VictimX = 500.0f;
    bool Teleported = false;
    int32_t OlderClientTickOffset = 1;  // 2 breaks the strict adjacency the rule requires
};

std::optional<Finding> Run(ShotCorrelatorCore& correlator, AimbotCore& aimbot, const Incident& incident)
{
    correlator.CaptureFrame(incident.Tick - 1, Frame(incident.VictimX, incident.Teleported));
    correlator.CaptureFrame(incident.Tick, Frame(incident.VictimX, incident.Teleported));

    aimbot.OnCommand(Attacker,
                     AimCmd(incident.Base, incident.Tick - incident.OlderClientTickOffset, incident.YawOlder));
    aimbot.OnCommand(Attacker, AimCmd(incident.Base + 1, incident.Tick, incident.YawShot));
    aimbot.OnSimulated(Attacker, incident.Base, incident.Tick - 1, Eye, Now);
    aimbot.OnSimulated(Attacker, incident.Base + 1, incident.Tick, Eye, Now);

    ShotView shot;
    shot.Slot = Attacker;
    shot.CmdNum = incident.Base + 1;
    shot.ServerTick = incident.Tick;
    shot.FireTick = incident.Tick;
    return aimbot.OnPlayerHurt(Attacker, Victim, shot, Now);
}
}  // namespace

TEST_CASE("The wide convergence branch counts at a snap over 10 degrees collapsing below a fifth of the error")
{
    ShotCorrelatorCore correlator;
    AimbotCore aimbot(correlator);
    // snap 12.1 degrees, error 15 -> 2.9, and 2.9 is just under 15 * 0.2.
    Run(correlator, aimbot, {.YawOlder = 15.0f, .YawShot = 2.9f});
    CHECK(aimbot.IncidentCount(Attacker) == 1);
}

TEST_CASE("The wide convergence branch stops just short when the error does not collapse far enough")
{
    ShotCorrelatorCore correlator;
    AimbotCore aimbot(correlator);
    // The snap still clears 10 degrees; 3.1 is just over 15 * 0.2, and 15 * 0.1 is far out of reach.
    Run(correlator, aimbot, {.YawOlder = 15.0f, .YawShot = 3.1f});
    CHECK(aimbot.IncidentCount(Attacker) == 0);
}

TEST_CASE("The tight convergence branch counts a smaller snap that collapses below a tenth of the error")
{
    ShotCorrelatorCore correlator;
    AimbotCore aimbot(correlator);
    // snap 7.3 degrees is under the wide branch's threshold; 0.7 < 8 * 0.1 carries it.
    Run(correlator, aimbot, {.YawOlder = 8.0f, .YawShot = 0.7f});
    CHECK(aimbot.IncidentCount(Attacker) == 1);
}

TEST_CASE("The tight convergence branch stops just short at a tenth of the error")
{
    ShotCorrelatorCore correlator;
    AimbotCore aimbot(correlator);
    Run(correlator, aimbot, {.YawOlder = 8.0f, .YawShot = 0.9f});
    CHECK(aimbot.IncidentCount(Attacker) == 0);
}

TEST_CASE("A snap of exactly five degrees is below both branches")
{
    ShotCorrelatorCore correlator;
    AimbotCore aimbot(correlator);
    Run(correlator, aimbot, {.YawOlder = 5.0f, .YawShot = 0.0f});
    CHECK(aimbot.IncidentCount(Attacker) == 0);
}

TEST_CASE("A gap in the command chain breaks the convergence walk")
{
    ShotCorrelatorCore correlator;
    AimbotCore aimbot(correlator);
    Run(correlator, aimbot, {.YawOlder = 12.0f, .YawShot = 1.9f, .OlderClientTickOffset = 2});
    // The pending evaluation waits one tick for the command after the shot, then gives up.
    aimbot.OnFrame(Attacker, 103, true, Now);
    CHECK(aimbot.IncidentCount(Attacker) == 0);
}

TEST_CASE("A target closer than a hundred units never counts")
{
    ShotCorrelatorCore correlator;
    AimbotCore aimbot(correlator);
    Run(correlator, aimbot, {.YawOlder = 12.0f, .YawShot = 1.9f, .VictimX = 50.0f});
    CHECK(aimbot.IncidentCount(Attacker) == 0);
}

TEST_CASE("A teleport inside the snap window rejects the incident")
{
    ShotCorrelatorCore correlator;
    AimbotCore aimbot(correlator);
    Run(correlator, aimbot, {.YawOlder = 12.0f, .YawShot = 1.9f, .Teleported = true});
    CHECK(aimbot.IncidentCount(Attacker) == 0);
}

TEST_CASE("A shot against a teammate never counts")
{
    ShotCorrelatorCore correlator;
    AimbotCore aimbot(correlator);
    auto players = Frame();
    players[Victim].Team = TeamT;
    correlator.CaptureFrame(99, players);
    correlator.CaptureFrame(100, players);
    aimbot.OnCommand(Attacker, AimCmd(100, 99, 12.0f));
    aimbot.OnCommand(Attacker, AimCmd(101, 100, 1.9f));
    aimbot.OnSimulated(Attacker, 100, 99, Eye, Now);
    aimbot.OnSimulated(Attacker, 101, 100, Eye, Now);
    ShotView shot;
    shot.Slot = Attacker;
    shot.CmdNum = 101;
    shot.ServerTick = 100;
    shot.FireTick = 100;
    aimbot.OnPlayerHurt(Attacker, Victim, shot, Now);
    CHECK(aimbot.IncidentCount(Attacker) == 0);
}

TEST_CASE("The fourth incident fires and the third does not")
{
    ShotCorrelatorCore correlator;
    AimbotCore aimbot(correlator);
    for (int i = 0; i < 3; ++i)
    {
        const int32_t base = 100 + 10 * i;
        CHECK_FALSE(Run(correlator, aimbot, {.Base = base, .Tick = base}).has_value());
    }
    CHECK(aimbot.IncidentCount(Attacker) == 3);

    const std::optional<Finding> finding = Run(correlator, aimbot, {.Base = 130, .Tick = 130});
    REQUIRE(finding.has_value());
    CHECK(finding->Kind == DetectionKind::Aimbot);
    CHECK_FALSE(finding->KickOnly);
    CHECK_FALSE(finding->Evidence.empty());
    // Firing clears the window so the next detection needs four fresh incidents.
    CHECK(aimbot.IncidentCount(Attacker) == 0);
}

TEST_CASE("Incidents outside the ten minute window fall out before the threshold is reached")
{
    ShotCorrelatorCore correlator;
    AimbotCore aimbot(correlator);
    for (int i = 0; i < 3; ++i)
    {
        const int32_t base = 100 + 10 * i;
        correlator.CaptureFrame(base - 1, Frame());
        correlator.CaptureFrame(base, Frame());
        aimbot.OnCommand(Attacker, AimCmd(base, base - 1, 12.0f));
        aimbot.OnCommand(Attacker, AimCmd(base + 1, base, 1.9f));
        aimbot.OnSimulated(Attacker, base, base - 1, Eye, Now);
        aimbot.OnSimulated(Attacker, base + 1, base, Eye, Now);
        ShotView shot;
        shot.Slot = Attacker;
        shot.CmdNum = base + 1;
        shot.ServerTick = base;
        shot.FireTick = base;
        aimbot.OnPlayerHurt(Attacker, Victim, shot, Now);
    }
    CHECK(aimbot.IncidentCount(Attacker) == 3);

    // Eleven minutes later the earlier three no longer support a detection.
    correlator.CaptureFrame(129, Frame());
    correlator.CaptureFrame(130, Frame());
    aimbot.OnCommand(Attacker, AimCmd(130, 129, 12.0f));
    aimbot.OnCommand(Attacker, AimCmd(131, 130, 1.9f));
    aimbot.OnSimulated(Attacker, 130, 129, Eye, Now + 660.0);
    aimbot.OnSimulated(Attacker, 131, 130, Eye, Now + 660.0);
    ShotView shot;
    shot.Slot = Attacker;
    shot.CmdNum = 131;
    shot.ServerTick = 130;
    shot.FireTick = 130;
    CHECK_FALSE(aimbot.OnPlayerHurt(Attacker, Victim, shot, Now + 660.0).has_value());
    CHECK(aimbot.IncidentCount(Attacker) == 1);
}

TEST_CASE("A one command excursion that returns to the surrounding angle counts as a snap return")
{
    ShotCorrelatorCore correlator;
    AimbotCore aimbot(correlator);
    for (int32_t tick = 99; tick <= 101; ++tick)
        correlator.CaptureFrame(tick, Frame());

    // 0 -> 5 -> 0.2 degrees: the neighbours agree, the shot alone jumps, and the jump is under the
    // convergence thresholds so only the snap-return rule can catch it.
    aimbot.OnCommand(Attacker, AimCmd(100, 99, 0.0f));
    aimbot.OnCommand(Attacker, AimCmd(101, 100, 5.0f));
    aimbot.OnSimulated(Attacker, 100, 99, Eye, Now);
    aimbot.OnSimulated(Attacker, 101, 100, Eye, Now);

    ShotView shot;
    shot.Slot = Attacker;
    shot.CmdNum = 101;
    shot.ServerTick = 100;
    shot.FireTick = 100;
    CHECK_FALSE(aimbot.OnPlayerHurt(Attacker, Victim, shot, Now).has_value());
    CHECK(aimbot.IncidentCount(Attacker) == 0);  // still waiting for the command after the shot

    aimbot.OnCommand(Attacker, AimCmd(102, 101, 0.2f));
    aimbot.OnSimulated(Attacker, 102, 101, Eye, Now);
    CHECK(aimbot.IncidentCount(Attacker) == 1);
}

TEST_CASE("A steady aim across the shot is not a snap return")
{
    ShotCorrelatorCore correlator;
    AimbotCore aimbot(correlator);
    for (int32_t tick = 99; tick <= 101; ++tick)
        correlator.CaptureFrame(tick, Frame());

    aimbot.OnCommand(Attacker, AimCmd(100, 99, 0.0f));
    aimbot.OnCommand(Attacker, AimCmd(101, 100, 0.3f));
    aimbot.OnSimulated(Attacker, 100, 99, Eye, Now);
    aimbot.OnSimulated(Attacker, 101, 100, Eye, Now);

    ShotView shot;
    shot.Slot = Attacker;
    shot.CmdNum = 101;
    shot.ServerTick = 100;
    shot.FireTick = 100;
    aimbot.OnPlayerHurt(Attacker, Victim, shot, Now);

    aimbot.OnCommand(Attacker, AimCmd(102, 101, 0.6f));
    aimbot.OnSimulated(Attacker, 102, 101, Eye, Now);
    CHECK(aimbot.IncidentCount(Attacker) == 0);
}

TEST_CASE("One shot never funds two incidents")
{
    ShotCorrelatorCore correlator;
    AimbotCore aimbot(correlator);
    correlator.CaptureFrame(99, Frame());
    correlator.CaptureFrame(100, Frame());
    aimbot.OnCommand(Attacker, AimCmd(100, 99, 12.0f));
    aimbot.OnCommand(Attacker, AimCmd(101, 100, 1.9f));
    aimbot.OnSimulated(Attacker, 100, 99, Eye, Now);
    aimbot.OnSimulated(Attacker, 101, 100, Eye, Now);

    ShotView shot;
    shot.Slot = Attacker;
    shot.CmdNum = 101;
    shot.ServerTick = 100;
    shot.FireTick = 100;
    aimbot.OnPlayerHurt(Attacker, Victim, shot, Now);
    aimbot.OnPlayerHurt(Attacker, Victim, shot, Now);  // the shot is already consumed
    CHECK(aimbot.IncidentCount(Attacker) == 1);
}
