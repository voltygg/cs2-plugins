#include "Detect/Rules/Aimbot.hpp"

#include <array>
#include <doctest/doctest.h>

using Anticheat::AimAngles;
using Anticheat::Rules::Aimbot;
using Anticheat::CmdSample;
using Anticheat::DetectionKind;
using Anticheat::Finding;
using Anticheat::MaxSlots;
using Anticheat::PositionSample;
using Anticheat::ShotHistory;
using Anticheat::ShotView;
using Anticheat::Suspicion;
using Anticheat::TeamCT;
using Anticheat::TeamT;
using Anticheat::Vec3;

static constexpr int Attacker = 0;
static constexpr int Victim = 1;
static constexpr double Now = 1000.0;
static constexpr Vec3 Eye{0.0f, 0.0f, 64.0f};

static Suspicion MakeScores()
{
    Suspicion scores;
    return scores;
}

/** Snap-hit incidents counted against the attacker, in this rule's own units. */
static float Incidents(const Suspicion& scores, double now = Now)
{
    return scores.Value(Attacker, DetectionKind::Aimbot, now) * 4.0f;
}

static std::array<PositionSample, MaxSlots> Frame(float victimX = 500.0f, bool teleported = false)
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

static CmdSample AimCmd(int32_t num, int32_t clientTick, float yaw)
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
 * One damaging shot preceded by one adjacent command. The eye is level with the victim's tallest
 * body point, so a level aim of y degrees is off target by exactly y: the yaws below *are* the
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
    bool VictimIsTeammate = false;
    double At = Now;
};

static std::optional<Finding> Run(ShotHistory& correlator, Aimbot& aimbot, const Incident& incident)
{
    auto players = Frame(incident.VictimX, incident.Teleported);
    if (incident.VictimIsTeammate)
        players[Victim].Team = TeamT;
    correlator.CaptureFrame(incident.Tick - 1, players);
    correlator.CaptureFrame(incident.Tick, players);

    aimbot.OnCommand(Attacker,
                     AimCmd(incident.Base, incident.Tick - incident.OlderClientTickOffset, incident.YawOlder));
    aimbot.OnCommand(Attacker, AimCmd(incident.Base + 1, incident.Tick, incident.YawShot));
    aimbot.OnSimulated(Attacker, incident.Base, incident.Tick - 1, Eye, incident.At);
    aimbot.OnSimulated(Attacker, incident.Base + 1, incident.Tick, Eye, incident.At);

    ShotView shot;
    shot.Slot = Attacker;
    shot.CmdNum = incident.Base + 1;
    shot.ServerTick = incident.Tick;
    shot.FireTick = incident.Tick;
    return aimbot.OnPlayerHurt(Attacker, Victim, shot, incident.At);
}

TEST_CASE("The wide convergence branch counts at a snap over 10 degrees collapsing below a fifth of the error")
{
    ShotHistory correlator;
    Suspicion scores = MakeScores();
    Aimbot aimbot(correlator, scores);
    // snap 12.1 degrees, error 15 -> 2.9, and 2.9 is just under 15 * 0.2.
    Run(correlator, aimbot, {.YawOlder = 15.0f, .YawShot = 2.9f});
    CHECK(Incidents(scores) == doctest::Approx(1.0f));
}

TEST_CASE("The wide convergence branch stops just short when the error does not collapse far enough")
{
    ShotHistory correlator;
    Suspicion scores = MakeScores();
    Aimbot aimbot(correlator, scores);
    // The snap still clears 10 degrees; 3.1 is just over 15 * 0.2, and 15 * 0.1 is far out of reach.
    Run(correlator, aimbot, {.YawOlder = 15.0f, .YawShot = 3.1f});
    CHECK(Incidents(scores) == doctest::Approx(0.0f));
}

TEST_CASE("The tight convergence branch counts a smaller snap that collapses below a tenth of the error")
{
    ShotHistory correlator;
    Suspicion scores = MakeScores();
    Aimbot aimbot(correlator, scores);
    // snap 7.3 degrees is under the wide branch's threshold; 0.7 < 8 * 0.1 carries it.
    Run(correlator, aimbot, {.YawOlder = 8.0f, .YawShot = 0.7f});
    CHECK(Incidents(scores) == doctest::Approx(1.0f));
}

TEST_CASE("The tight convergence branch stops just short at a tenth of the error")
{
    ShotHistory correlator;
    Suspicion scores = MakeScores();
    Aimbot aimbot(correlator, scores);
    Run(correlator, aimbot, {.YawOlder = 8.0f, .YawShot = 0.9f});
    CHECK(Incidents(scores) == doctest::Approx(0.0f));
}

TEST_CASE("A snap of exactly five degrees is below both branches")
{
    ShotHistory correlator;
    Suspicion scores = MakeScores();
    Aimbot aimbot(correlator, scores);
    Run(correlator, aimbot, {.YawOlder = 5.0f, .YawShot = 0.0f});
    CHECK(Incidents(scores) == doctest::Approx(0.0f));
}

TEST_CASE("A gap in the command chain breaks the convergence walk")
{
    ShotHistory correlator;
    Suspicion scores = MakeScores();
    Aimbot aimbot(correlator, scores);
    Run(correlator, aimbot, {.YawOlder = 12.0f, .YawShot = 1.9f, .OlderClientTickOffset = 2});
    // The pending evaluation waits one tick for the command after the shot, then gives up.
    aimbot.OnFrame(Attacker, 103, true, Now);
    CHECK(Incidents(scores) == doctest::Approx(0.0f));
}

TEST_CASE("A target closer than a hundred units never counts")
{
    ShotHistory correlator;
    Suspicion scores = MakeScores();
    Aimbot aimbot(correlator, scores);
    Run(correlator, aimbot, {.YawOlder = 12.0f, .YawShot = 1.9f, .VictimX = 50.0f});
    CHECK(Incidents(scores) == doctest::Approx(0.0f));
}

TEST_CASE("A teleport inside the snap window rejects the incident")
{
    ShotHistory correlator;
    Suspicion scores = MakeScores();
    Aimbot aimbot(correlator, scores);
    Run(correlator, aimbot, {.YawOlder = 12.0f, .YawShot = 1.9f, .Teleported = true});
    CHECK(Incidents(scores) == doctest::Approx(0.0f));
}

TEST_CASE("A shot against a teammate never counts")
{
    ShotHistory correlator;
    Suspicion scores = MakeScores();
    Aimbot aimbot(correlator, scores);
    Run(correlator, aimbot, {.VictimIsTeammate = true});
    CHECK(Incidents(scores) == doctest::Approx(0.0f));
}

TEST_CASE("Incidents accumulate and the fourth is what this rule reports on alone")
{
    ShotHistory correlator;
    Suspicion scores = MakeScores();
    Aimbot aimbot(correlator, scores);
    for (int i = 0; i < 3; ++i)
    {
        const int32_t base = 100 + 10 * i;
        CHECK_FALSE(Run(correlator, aimbot, {.Base = base, .Tick = base}).has_value());
    }
    CHECK(Incidents(scores) == doctest::Approx(3.0f));

    const std::optional<Finding> finding = Run(correlator, aimbot, {.Base = 130, .Tick = 130});
    REQUIRE(finding.has_value());
    CHECK(finding->Kind == DetectionKind::Aimbot);
    CHECK_FALSE(finding->KickOnly);
    CHECK_FALSE(finding->Evidence.empty());
    CHECK(Incidents(scores) == doctest::Approx(4.0f));
}

TEST_CASE("Evidence survives a player going ineligible mid shot")
{
    ShotHistory correlator;
    Suspicion scores = MakeScores();
    Aimbot aimbot(correlator, scores);
    Run(correlator, aimbot, {});
    REQUIRE(Incidents(scores) == doctest::Approx(1.0f));

    // Dropping the shot waiting to be judged must not take the evidence already earned with it.
    aimbot.OnFrame(Attacker, 200, false, Now);
    CHECK(Incidents(scores) == doctest::Approx(1.0f));
}

TEST_CASE("A one command excursion that returns to the surrounding angle counts as a snap return")
{
    ShotHistory correlator;
    Suspicion scores = MakeScores();
    Aimbot aimbot(correlator, scores);
    for (int32_t tick = 99; tick <= 101; ++tick)
        correlator.CaptureFrame(tick, Frame());

    // 0 -> 5 -> 0.2 degrees: the neighbours agree and the shot alone jumps, but under the
    // convergence thresholds, so only the snap-return rule can catch it.
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
    CHECK(Incidents(scores) == doctest::Approx(0.0f));  // still waiting for the command after the shot

    aimbot.OnCommand(Attacker, AimCmd(102, 101, 0.2f));
    aimbot.OnSimulated(Attacker, 102, 101, Eye, Now);
    CHECK(Incidents(scores) == doctest::Approx(1.0f));
}

TEST_CASE("A steady aim across the shot is not a snap return")
{
    ShotHistory correlator;
    Suspicion scores = MakeScores();
    Aimbot aimbot(correlator, scores);
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
    CHECK(Incidents(scores) == doctest::Approx(0.0f));
}

TEST_CASE("One shot never funds two incidents")
{
    ShotHistory correlator;
    Suspicion scores = MakeScores();
    Aimbot aimbot(correlator, scores);
    Run(correlator, aimbot, {});

    ShotView shot;
    shot.Slot = Attacker;
    shot.CmdNum = 101;
    shot.ServerTick = 100;
    shot.FireTick = 100;
    aimbot.OnPlayerHurt(Attacker, Victim, shot, Now);  // the shot is already consumed
    CHECK(Incidents(scores) == doctest::Approx(1.0f));
}
