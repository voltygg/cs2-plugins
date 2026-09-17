#include "Detect/Rules/AimAssist.hpp"
#include "Detect/Geometry.hpp"

#include <array>
#include <cmath>
#include <doctest/doctest.h>

using Anticheat::AimAngles;
using Anticheat::ButtonTurnRight;
using Anticheat::CmdSample;
using Anticheat::DefaultTuning;
using Anticheat::DetectionKind;
using Anticheat::MaxSlots;
using Anticheat::Rules::AimAssist;
using Anticheat::PositionSample;
using Anticheat::ShotHistory;
using Anticheat::Suspicion;
using Anticheat::TeamCT;
using Anticheat::TeamT;
using Anticheat::Vec3;

static constexpr int Observer = 0;
static constexpr int Target = 1;
static constexpr double Now = 100.0;
static constexpr Vec3 Eye{0.0f, 0.0f, 64.0f};
/** Five degrees to the left of a zero yaw: tan(5 deg) * 500. */
static constexpr float TargetY = 43.74f;
/** Degrees per count for m_yaw 0.022 at sensitivity 2. Turning right lowers the yaw. */
static constexpr float Scale = -0.044f;

struct AimAssistHarness
{
    AimAssistHarness() { Scores.Configure(DefaultTuning()); }

    /** Unexplained turns this rule has counted on the observer. */
    float Turns() const { return Scores.Value(Observer, DetectionKind::AimAssist, Now); }

    Suspicion Scores;
    ShotHistory History;
    AimAssist Rule{History, Scores};
    int32_t Tick = 0;
    int32_t Cmd = 1;
    float Yaw = 0.0f;
    int Findings = 0;

    std::array<PositionSample, MaxSlots> Frame() const
    {
        std::array<PositionSample, MaxSlots> players{};
        players[Observer] = {.Origin = {0.0f, 0.0f, 0.0f}, .EyePos = Eye, .Team = TeamT, .Valid = true, .Alive = true};
        players[Target] = {.Origin = {500.0f, TargetY, 0.0f},
                           .EyePos = {500.0f, TargetY, 64.0f},
                           .Team = TeamCT,
                           .Valid = true,
                           .Alive = true};
        return players;
    }

    /** One command: the frame before it is captured, then the view lands on @p yaw. */
    void Command(int dx, float yaw, uint64_t buttons = 0, bool scoped = false)
    {
        History.CaptureFrame(Tick, Frame());
        ++Tick;
        Yaw = yaw;
        CmdSample cmd;
        cmd.CmdNum = Cmd++;
        cmd.ClientTick = Tick;
        cmd.ViewYaw = Yaw;
        cmd.MouseDx = dx;
        cmd.Buttons = buttons;
        cmd.Scoped = scoped;
        cmd.EyePos = Eye;
        if (Rule.OnSimulated(Observer, cmd, Tick, false, Now))
            ++Findings;
    }

    /** A mouse turn of @p dx counts, as the client would report it. */
    void Turn(int dx) { Command(dx, Yaw + static_cast<float>(dx) * Scale); }

    void Calibrate()
    {
        for (int i = 0; i < 20; ++i)
            Turn(i % 2 == 0 ? 10 : -10);
    }
};

TEST_CASE("Turns the mouse counts cannot explain that land on an enemy are counted")
{
    AimAssistHarness h;
    h.Calibrate();
    CHECK(h.Rule.Calibrated(Observer));

    for (int i = 0; i < 5; ++i)
    {
        h.Command(0, 5.0f);  // onto the enemy with the mouse at rest
        h.Turn(114);         // back to zero by hand
    }
    CHECK(h.Findings == 0);
    CHECK(h.Turns() == doctest::Approx(5.0f));
    h.Command(0, 5.0f);
    CHECK(h.Findings == 1);
}

TEST_CASE("The same turn reported by the mouse is ordinary aim")
{
    AimAssistHarness h;
    h.Calibrate();
    for (int i = 0; i < 6; ++i)
    {
        h.Turn(-114);
        h.Turn(114);
    }
    CHECK(h.Findings == 0);
    CHECK(h.Turns() == doctest::Approx(0.0f));
}

TEST_CASE("An unexplained turn away from every enemy is not evidence")
{
    AimAssistHarness h;
    h.Calibrate();
    for (int i = 0; i < 6; ++i)
    {
        h.Command(0, -5.0f);
        h.Turn(-114);
    }
    CHECK(h.Turns() == doctest::Approx(0.0f));
}

TEST_CASE("A client whose counts never agree with its turns is never judged")
{
    AimAssistHarness h;
    for (int i = 0; i < 20; ++i)
        h.Command(i % 2 == 0 ? 10 : -3, h.Yaw + (i % 2 == 0 ? 0.44f : -0.44f));
    CHECK_FALSE(h.Rule.Calibrated(Observer));
    for (int i = 0; i < 6; ++i)
    {
        h.Command(0, 5.0f);
        h.Command(0, 0.0f);
    }
    CHECK(h.Turns() == doctest::Approx(0.0f));
}

TEST_CASE("Keyboard turning and scoped commands are skipped")
{
    AimAssistHarness h;
    h.Calibrate();
    for (int i = 0; i < 6; ++i)
    {
        h.Command(0, 5.0f, ButtonTurnRight);
        h.Turn(114);
        h.Command(0, 5.0f, 0, true);
        h.Turn(114);
    }
    CHECK(h.Turns() == doctest::Approx(0.0f));
}
