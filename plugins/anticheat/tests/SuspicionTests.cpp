#include "Detect/Suspicion.hpp"
#include "Harness.hpp"

#include <doctest/doctest.h>

#include <utility>

using Anticheat::Confidence;
using Anticheat::Contribution;
using Anticheat::DetectionKind;
using Anticheat::FadesOverTheSession;
using Anticheat::Finding;
using Anticheat::MaxSlots;
using Anticheat::Suspicion;

static constexpr int Slot = 3;
static constexpr double Now = 1000.0;

/** A rule that reports on its own after four incidents contributes a quarter each time. */
static constexpr float Quarter = 1.0f / 4.0f;
static constexpr float Sixth = 1.0f / 6.0f;

static Contribution Share(DetectionKind kind, float points)
{
    return {.Kind = kind, .Points = points, .Reason = "Because."};
}

/** A score with its sink wired, so a case can read back what a contribution reported. */
struct Scored
{
    Anticheat::Test::Findings Reported;
    Suspicion Score;

    Scored() { Score.ReportTo(Reported.Sink()); }

    /** Mirrors Suspicion::Add, handing back the finding it reported. */
    std::optional<Finding> Add(int slot, const Contribution& contribution, double nowSec)
    {
        Reported.Last.reset();
        Score.Add(slot, contribution, nowSec);
        return Reported.Last;
    }
};

TEST_CASE("A rule reaching its own threshold alone is exactly one suspicion")
{
    Scored scored;
    for (int i = 0; i < 3; ++i)
        CHECK_FALSE(scored.Add(Slot, Share(DetectionKind::Aimbot, Quarter), Now).has_value());
    CHECK(scored.Score.Total(Slot, Now) == doctest::Approx(0.75f));

    const std::optional<Finding> finding = scored.Add(Slot, Share(DetectionKind::Aimbot, Quarter), Now);
    REQUIRE(finding.has_value());
    CHECK(finding->Kind == DetectionKind::Aimbot);
    CHECK(finding->Level == Confidence::Suspect);
    CHECK(finding->Suspicion == doctest::Approx(1.0f));
}

TEST_CASE("Reporting does not clear the score")
{
    Scored scored;
    for (int i = 0; i < 4; ++i)
        scored.Add(Slot, Share(DetectionKind::Aimbot, Quarter), Now);

    // The old rolling window zeroed itself here, handing the cheat a clean ten minutes.
    CHECK(scored.Score.Value(Slot, DetectionKind::Aimbot, Now) == doctest::Approx(1.0f));
}

TEST_CASE("Two rules that each stay under their own threshold still raise a finding together")
{
    Scored scored;
    for (int i = 0; i < 3; ++i)
        CHECK_FALSE(scored.Add(Slot, Share(DetectionKind::Aimbot, Quarter), Now).has_value());
    CHECK_FALSE(scored.Add(Slot, Share(DetectionKind::Wallhack, Sixth), Now).has_value());

    // Three quarters of aimbot plus a third of wallhack: neither would ever have reported alone.
    const std::optional<Finding> finding = scored.Add(Slot, Share(DetectionKind::Wallhack, Sixth), Now);
    REQUIRE(finding.has_value());
    CHECK(finding->Kind == DetectionKind::Wallhack);
    CHECK(finding->Suspicion == doctest::Approx(0.75f + 2.0f * Sixth));
    CHECK(finding->Evidence.find("aimbot 0.75") != std::string::npos);
    CHECK(finding->Evidence.find("wallhack 0.33") != std::string::npos);
}

TEST_CASE("Each band reports once")
{
    Scored scored;
    int findings = 0;
    for (int i = 0; i < 24; ++i)
        if (scored.Add(Slot, Share(DetectionKind::Aimbot, Quarter), Now))
            ++findings;
    // Suspect at one, likely at two, certain at three, and nothing more however far it climbs.
    CHECK(findings == 3);
}

TEST_CASE("A band speaks again only after the score decays below it")
{
    Scored scored;
    REQUIRE(scored.Add(Slot, Share(DetectionKind::Aimbot, 1.0f), Now).has_value());
    CHECK_FALSE(scored.Add(Slot, Share(DetectionKind::Aimbot, 0.025f), Now).has_value());

    // Two half-lives leave a quarter, well under 0.75 of the band, so suspect opens again.
    const double later = Now + 1200.0;
    const std::optional<Finding> again = scored.Add(Slot, Share(DetectionKind::Aimbot, 0.75f), later);
    REQUIRE(again.has_value());
    CHECK(again->Level == Confidence::Suspect);
}

TEST_CASE("Fused evidence alerts but never punishes on its own")
{
    Scored scored;
    // Four rules at nine tenths of their own threshold each: 3.6 suspicion, nobody confident alone.
    const std::pair<DetectionKind, float> evidence[] = {
        {DetectionKind::Aimbot, 0.9f},
        {DetectionKind::Wallhack, 0.9f},
        {DetectionKind::Recoil, 0.9f},
        {DetectionKind::Triggerbot, 0.9f},
    };

    Confidence highest = Confidence::Suspect;
    for (const auto& [kind, points] : evidence)
        if (const std::optional<Finding> finding = scored.Add(Slot, Share(kind, points), Now))
            highest = finding->Level;

    // The total is past certain, but no single rule is confident, so the band is held at likely.
    CHECK(scored.Score.Total(Slot, Now) == doctest::Approx(3.6f));
    CHECK(highest == Confidence::Likely);
}

TEST_CASE("One rule confident on its own reaches certain")
{
    Scored scored;
    std::optional<Finding> finding;
    for (int i = 0; i < 12; ++i)
        if (auto next = scored.Add(Slot, Share(DetectionKind::Aimbot, Quarter), Now))
            finding = next;
    REQUIRE(finding.has_value());
    CHECK(finding->Level == Confidence::Certain);
}

TEST_CASE("Evidence fades on the half-life the rule asked for")
{
    Scored scored;
    scored.Add(Slot, Share(DetectionKind::Aimbot, 1.0f), Now);
    CHECK(scored.Score.Value(Slot, DetectionKind::Aimbot, Now + 600.0) == doctest::Approx(0.5f));

    // A confirmed fact about the client holds its weight for far longer than an aim incident.
    Contribution client = Share(DetectionKind::DllInjection, 1.0f);
    client.HalfLifeSec = FadesOverTheSession;
    scored.Add(Slot, client, Now);
    CHECK(scored.Score.Value(Slot, DetectionKind::DllInjection, Now + 600.0) == doctest::Approx(0.891f).epsilon(0.01));
}

TEST_CASE("Evidence stays with the slot until it changes hands")
{
    Scored scored;
    scored.Add(Slot, Share(DetectionKind::Aimbot, 0.75f), Now);
    CHECK(scored.Score.Value(Slot, DetectionKind::Aimbot, Now) == doctest::Approx(0.75f));

    scored.Score.ClearSlot(Slot + 1);
    CHECK(scored.Score.Value(Slot, DetectionKind::Aimbot, Now) == doctest::Approx(0.75f));

    scored.Score.ClearSlot(Slot);
    CHECK(scored.Score.Value(Slot, DetectionKind::Aimbot, Now) == doctest::Approx(0.0f));
}

TEST_CASE("Evidence saved from one slot comes back whole in another")
{
    Scored scored;
    REQUIRE(scored.Add(Slot, Share(DetectionKind::Aimbot, 1.0f), Now).has_value());
    const Anticheat::PlayerEvidence held = scored.Score.Save(Slot);
    scored.Score.ClearSlot(Slot);
    REQUIRE(scored.Score.Total(Slot, Now) == doctest::Approx(0.0f));

    // A reconnect puts the player in whatever slot is free; the evidence has to follow them.
    scored.Score.Restore(Slot + 1, held);
    CHECK(scored.Score.Value(Slot + 1, DetectionKind::Aimbot, Now) == doctest::Approx(1.0f));
    // The band they already reached comes with it, so returning does not re-report what it did.
    CHECK_FALSE(scored.Add(Slot + 1, Share(DetectionKind::Aimbot, 0.05f), Now).has_value());
}

TEST_CASE("Out of range slots are ignored rather than indexed")
{
    Scored scored;
    CHECK_FALSE(scored.Add(-1, Share(DetectionKind::Aimbot, 1.0f), Now).has_value());
    CHECK_FALSE(scored.Add(MaxSlots, Share(DetectionKind::Aimbot, 1.0f), Now).has_value());
    CHECK(scored.Score.Value(-1, DetectionKind::Aimbot, Now) == doctest::Approx(0.0f));
    CHECK(scored.Score.Total(MaxSlots, Now) == doctest::Approx(0.0f));
}

