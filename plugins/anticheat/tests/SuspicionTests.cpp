#include "Detect/Suspicion.hpp"

#include <doctest/doctest.h>

#include <utility>

using Anticheat::Confidence;
using Anticheat::Contribution;
using Anticheat::DetectionKind;
using Anticheat::FadesOverTheSession;
using Anticheat::Finding;
using Anticheat::MaxSlots;
using Anticheat::Suspicion;
using Anticheat::SuspicionBands;

static constexpr int Slot = 3;
static constexpr double Now = 1000.0;

/** A rule that reports on its own after four incidents contributes a quarter each time. */
static constexpr float Quarter = 1.0f / 4.0f;
static constexpr float Sixth = 1.0f / 6.0f;

static Suspicion Configured()
{
    Suspicion suspicion;
    suspicion.Configure({});
    return suspicion;
}

static Contribution Share(DetectionKind kind, float points)
{
    return {.Kind = kind, .Points = points, .Reason = "Because."};
}

TEST_CASE("A rule reaching its own threshold alone is exactly one suspicion")
{
    Suspicion suspicion = Configured();
    for (int i = 0; i < 3; ++i)
        CHECK_FALSE(suspicion.Add(Slot, Share(DetectionKind::Aimbot, Quarter), Now).has_value());
    CHECK(suspicion.Total(Slot, Now) == doctest::Approx(0.75f));

    const std::optional<Finding> finding = suspicion.Add(Slot, Share(DetectionKind::Aimbot, Quarter), Now);
    REQUIRE(finding.has_value());
    CHECK(finding->Kind == DetectionKind::Aimbot);
    CHECK(finding->Level == Confidence::Suspect);
    CHECK(finding->Suspicion == doctest::Approx(1.0f));
}

TEST_CASE("Reporting does not clear the score")
{
    Suspicion suspicion = Configured();
    for (int i = 0; i < 4; ++i)
        suspicion.Add(Slot, Share(DetectionKind::Aimbot, Quarter), Now);

    // The old rolling window zeroed itself here, handing the cheat a clean ten minutes.
    CHECK(suspicion.Value(Slot, DetectionKind::Aimbot, Now) == doctest::Approx(1.0f));
}

TEST_CASE("Two rules that each stay under their own threshold still raise a finding together")
{
    Suspicion suspicion = Configured();
    for (int i = 0; i < 3; ++i)
        CHECK_FALSE(suspicion.Add(Slot, Share(DetectionKind::Aimbot, Quarter), Now).has_value());
    CHECK_FALSE(suspicion.Add(Slot, Share(DetectionKind::Wallhack, Sixth), Now).has_value());

    // Three quarters of aimbot plus a third of wallhack: neither would ever have reported alone.
    const std::optional<Finding> finding = suspicion.Add(Slot, Share(DetectionKind::Wallhack, Sixth), Now);
    REQUIRE(finding.has_value());
    CHECK(finding->Kind == DetectionKind::Wallhack);
    CHECK(finding->Suspicion == doctest::Approx(0.75f + 2.0f * Sixth));
    CHECK(finding->Evidence.find("aimbot 0.75") != std::string::npos);
    CHECK(finding->Evidence.find("wallhack 0.33") != std::string::npos);
}

TEST_CASE("Each band reports once")
{
    Suspicion suspicion = Configured();
    int findings = 0;
    for (int i = 0; i < 24; ++i)
        if (suspicion.Add(Slot, Share(DetectionKind::Aimbot, Quarter), Now))
            ++findings;
    // Suspect at one, likely at two, certain at three, and nothing more however far it climbs.
    CHECK(findings == 3);
}

TEST_CASE("A band speaks again only after the score decays below it")
{
    Suspicion suspicion = Configured();
    REQUIRE(suspicion.Add(Slot, Share(DetectionKind::Aimbot, 1.0f), Now).has_value());
    CHECK_FALSE(suspicion.Add(Slot, Share(DetectionKind::Aimbot, 0.025f), Now).has_value());

    // Two half-lives leave a quarter, well under 0.75 of the band, so suspect opens again.
    const double later = Now + 1200.0;
    const std::optional<Finding> again = suspicion.Add(Slot, Share(DetectionKind::Aimbot, 0.75f), later);
    REQUIRE(again.has_value());
    CHECK(again->Level == Confidence::Suspect);
}

TEST_CASE("Fused evidence alerts but never punishes on its own")
{
    Suspicion suspicion = Configured();
    // Four rules at nine tenths of their own threshold each: 3.6 suspicion, nobody confident alone.
    const std::pair<DetectionKind, float> evidence[] = {
        {DetectionKind::Aimbot, 0.9f},
        {DetectionKind::Wallhack, 0.9f},
        {DetectionKind::Recoil, 0.9f},
        {DetectionKind::Triggerbot, 0.9f},
    };

    Confidence highest = Confidence::Suspect;
    for (const auto& [kind, points] : evidence)
        if (const std::optional<Finding> finding = suspicion.Add(Slot, Share(kind, points), Now))
            highest = finding->Level;

    // The total is past certain, but no single rule is confident, so the band is held at likely.
    CHECK(suspicion.Total(Slot, Now) == doctest::Approx(3.6f));
    CHECK(highest == Confidence::Likely);
}

TEST_CASE("One rule confident on its own reaches certain")
{
    Suspicion suspicion = Configured();
    std::optional<Finding> finding;
    for (int i = 0; i < 12; ++i)
        if (auto next = suspicion.Add(Slot, Share(DetectionKind::Aimbot, Quarter), Now))
            finding = next;
    REQUIRE(finding.has_value());
    CHECK(finding->Level == Confidence::Certain);
}

TEST_CASE("Evidence fades on the half-life the rule asked for")
{
    Suspicion suspicion = Configured();
    suspicion.Add(Slot, Share(DetectionKind::Aimbot, 1.0f), Now);
    CHECK(suspicion.Value(Slot, DetectionKind::Aimbot, Now + 600.0) == doctest::Approx(0.5f));

    // A confirmed fact about the client holds its weight for far longer than an aim incident.
    Contribution client = Share(DetectionKind::DllInjection, 1.0f);
    client.HalfLifeSec = FadesOverTheSession;
    suspicion.Add(Slot, client, Now);
    CHECK(suspicion.Value(Slot, DetectionKind::DllInjection, Now + 600.0) == doctest::Approx(0.891f).epsilon(0.01));
}

TEST_CASE("Configure clamps unusable bands and names what it rejected")
{
    SuspicionBands bands;
    bands.Likely = 0.5f;  // below suspect
    bands.ReportAgainBelow = 1.5f;

    Suspicion suspicion;
    const std::vector<std::string_view> rejected = suspicion.Configure(bands);
    REQUIRE(rejected.size() == 2);
    CHECK(rejected[0] == "bands");
    CHECK(rejected[1] == "reportAgainBelow");

    // The compiled bands stand, so half a unit still reports nothing.
    CHECK_FALSE(suspicion.Add(Slot, Share(DetectionKind::Aimbot, 0.5f), Now).has_value());
}

TEST_CASE("Evidence stays with the slot until it changes hands")
{
    Suspicion suspicion = Configured();
    suspicion.Add(Slot, Share(DetectionKind::Aimbot, 0.75f), Now);
    CHECK(suspicion.Value(Slot, DetectionKind::Aimbot, Now) == doctest::Approx(0.75f));

    suspicion.OnSlotChanged(Slot + 1);
    CHECK(suspicion.Value(Slot, DetectionKind::Aimbot, Now) == doctest::Approx(0.75f));

    suspicion.OnSlotChanged(Slot);
    CHECK(suspicion.Value(Slot, DetectionKind::Aimbot, Now) == doctest::Approx(0.0f));
}

TEST_CASE("Out of range slots are ignored rather than indexed")
{
    Suspicion suspicion = Configured();
    CHECK_FALSE(suspicion.Add(-1, Share(DetectionKind::Aimbot, 1.0f), Now).has_value());
    CHECK_FALSE(suspicion.Add(MaxSlots, Share(DetectionKind::Aimbot, 1.0f), Now).has_value());
    CHECK(suspicion.Value(-1, DetectionKind::Aimbot, Now) == doctest::Approx(0.0f));
    CHECK(suspicion.Total(MaxSlots, Now) == doctest::Approx(0.0f));
}

TEST_CASE("The top contributor names the rule carrying the most weight")
{
    Suspicion suspicion = Configured();
    suspicion.Add(Slot, Share(DetectionKind::Aimbot, 0.5f), Now);
    suspicion.Add(Slot, Share(DetectionKind::Wallhack, 0.83f), Now);
    CHECK(suspicion.TopContributor(Slot, Now) == DetectionKind::Wallhack);
}
