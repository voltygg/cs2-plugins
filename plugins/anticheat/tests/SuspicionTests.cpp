#include "Detect/Suspicion.hpp"

#include <doctest/doctest.h>

#include <utility>

using Anticheat::Confidence;
using Anticheat::Contribution;
using Anticheat::DetectionKind;
using Anticheat::Finding;
using Anticheat::KindTuning;
using Anticheat::MaxSlots;
using Anticheat::Suspicion;
using Anticheat::SuspicionTuning;

static constexpr int Slot = 3;
static constexpr double Now = 1000.0;

/** Aimbot needs four incidents alone and wallhack six points, exactly as they do today. */
static SuspicionTuning Tuning()
{
    SuspicionTuning tuning;
    for (KindTuning& kind : tuning.Kinds)
        kind = {.Enabled = true, .ConfidentAlone = 1.0f, .HalfLifeSec = 600.0f};
    tuning.Kinds[static_cast<size_t>(DetectionKind::Aimbot)].ConfidentAlone = 4.0f;
    tuning.Kinds[static_cast<size_t>(DetectionKind::Wallhack)].ConfidentAlone = 6.0f;
    return tuning;
}

static Suspicion Configured()
{
    Suspicion suspicion;
    suspicion.Configure(Tuning());
    return suspicion;
}

static Contribution Points(DetectionKind kind, float points)
{
    return {.Kind = kind, .Points = points, .Reason = "Because."};
}

TEST_CASE("A rule reaching its own threshold alone is exactly one suspicion")
{
    Suspicion suspicion = Configured();
    for (int i = 0; i < 3; ++i)
        CHECK_FALSE(suspicion.Add(Slot, Points(DetectionKind::Aimbot, 1.0f), Now).has_value());
    CHECK(suspicion.Value(Slot, DetectionKind::Aimbot, Now) == doctest::Approx(3.0f));
    CHECK(suspicion.Total(Slot, Now) == doctest::Approx(0.75f));

    const std::optional<Finding> finding = suspicion.Add(Slot, Points(DetectionKind::Aimbot, 1.0f), Now);
    REQUIRE(finding.has_value());
    CHECK(finding->Kind == DetectionKind::Aimbot);
    CHECK(finding->Level == Confidence::Suspect);
    CHECK(finding->Suspicion == doctest::Approx(1.0f));
    CHECK(suspicion.Total(Slot, Now) == doctest::Approx(1.0f));
}

TEST_CASE("Reporting does not clear the score")
{
    Suspicion suspicion = Configured();
    for (int i = 0; i < 4; ++i)
        suspicion.Add(Slot, Points(DetectionKind::Aimbot, 1.0f), Now);

    // The old rolling window zeroed itself here, handing the cheat a clean ten minutes.
    CHECK(suspicion.Value(Slot, DetectionKind::Aimbot, Now) == doctest::Approx(4.0f));
}

TEST_CASE("Two rules that each stay under their own threshold still raise a finding together")
{
    Suspicion suspicion = Configured();
    for (int i = 0; i < 3; ++i)
        CHECK_FALSE(suspicion.Add(Slot, Points(DetectionKind::Aimbot, 1.0f), Now).has_value());
    CHECK_FALSE(suspicion.Add(Slot, Points(DetectionKind::Wallhack, 1.0f), Now).has_value());

    // Three quarters of aimbot plus a third of wallhack: neither would ever have reported alone.
    const std::optional<Finding> finding = suspicion.Add(Slot, Points(DetectionKind::Wallhack, 1.0f), Now);
    REQUIRE(finding.has_value());
    CHECK(finding->Kind == DetectionKind::Wallhack);
    CHECK(finding->Suspicion == doctest::Approx(1.0f + 1.0f / 12.0f));
    CHECK(finding->Evidence.find("aimbot 0.75") != std::string::npos);
    CHECK(finding->Evidence.find("wallhack 0.33") != std::string::npos);
}

TEST_CASE("Each band reports once")
{
    Suspicion suspicion = Configured();
    int findings = 0;
    for (int i = 0; i < 24; ++i)
        if (suspicion.Add(Slot, Points(DetectionKind::Aimbot, 1.0f), Now))
            ++findings;
    // Suspect at one, likely at two, certain at three, and nothing more however far it climbs.
    CHECK(findings == 3);
}

TEST_CASE("A band speaks again only after the score decays below it")
{
    Suspicion suspicion = Configured();
    REQUIRE(suspicion.Add(Slot, Points(DetectionKind::Aimbot, 4.0f), Now).has_value());
    CHECK_FALSE(suspicion.Add(Slot, Points(DetectionKind::Aimbot, 0.1f), Now).has_value());

    // Two half-lives leave a quarter, well under 0.75 of the band, so suspect opens again.
    const double later = Now + 1200.0;
    const std::optional<Finding> again = suspicion.Add(Slot, Points(DetectionKind::Aimbot, 3.0f), later);
    REQUIRE(again.has_value());
    CHECK(again->Level == Confidence::Suspect);
}

TEST_CASE("Fused evidence alerts but never punishes on its own")
{
    Suspicion suspicion = Configured();
    // Four rules at nine tenths of their own threshold each: 3.6 suspicion, nobody confident alone.
    const std::pair<DetectionKind, float> evidence[] = {
        {DetectionKind::Aimbot, 3.6f},
        {DetectionKind::Wallhack, 5.4f},
        {DetectionKind::Recoil, 0.9f},
        {DetectionKind::Triggerbot, 0.9f},
    };

    Confidence highest = Confidence::Suspect;
    for (const auto& [kind, points] : evidence)
        if (const std::optional<Finding> finding = suspicion.Add(Slot, Points(kind, points), Now))
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
        if (auto next = suspicion.Add(Slot, Points(DetectionKind::Aimbot, 1.0f), Now))
            finding = next;
    REQUIRE(finding.has_value());
    CHECK(finding->Level == Confidence::Certain);
}

TEST_CASE("Points decay by half-life")
{
    Suspicion suspicion = Configured();
    suspicion.Add(Slot, Points(DetectionKind::Aimbot, 4.0f), Now);
    CHECK(suspicion.Value(Slot, DetectionKind::Aimbot, Now + 600.0) == doctest::Approx(2.0f));
    CHECK(suspicion.Total(Slot, Now + 600.0) == doctest::Approx(0.5f));
}

TEST_CASE("A disabled rule contributes nothing")
{
    SuspicionTuning tuning = Tuning();
    tuning.Kinds[static_cast<size_t>(DetectionKind::Aimbot)].Enabled = false;
    Suspicion suspicion;
    suspicion.Configure(tuning);

    for (int i = 0; i < 20; ++i)
        CHECK_FALSE(suspicion.Add(Slot, Points(DetectionKind::Aimbot, 1.0f), Now).has_value());
    CHECK(suspicion.Total(Slot, Now) == doctest::Approx(0.0f));
}

TEST_CASE("Configure clamps unusable tuning and names what it rejected")
{
    SuspicionTuning tuning = Tuning();
    tuning.Kinds[static_cast<size_t>(DetectionKind::Aimbot)].ConfidentAlone = 0.0f;
    tuning.Likely = 0.5f;  // below suspect
    tuning.ReportAgainBelow = 1.5f;

    Suspicion suspicion;
    const std::vector<std::string_view> rejected = suspicion.Configure(tuning);
    REQUIRE(rejected.size() == 3);
    CHECK(rejected[0] == "aimbot");
    CHECK(rejected[1] == "bands");
    CHECK(rejected[2] == "reportAgainBelow");

    // A zero divisor would otherwise report everyone on their first incident.
    CHECK_FALSE(suspicion.Add(Slot, Points(DetectionKind::Aimbot, 0.5f), Now).has_value());
}

TEST_CASE("Evidence stays with the slot until it changes hands")
{
    Suspicion suspicion = Configured();
    suspicion.Add(Slot, Points(DetectionKind::Aimbot, 3.0f), Now);
    CHECK(suspicion.Value(Slot, DetectionKind::Aimbot, Now) == doctest::Approx(3.0f));

    suspicion.OnSlotChanged(Slot + 1);
    CHECK(suspicion.Value(Slot, DetectionKind::Aimbot, Now) == doctest::Approx(3.0f));

    suspicion.OnSlotChanged(Slot);
    CHECK(suspicion.Value(Slot, DetectionKind::Aimbot, Now) == doctest::Approx(0.0f));
}

TEST_CASE("Out of range slots are ignored rather than indexed")
{
    Suspicion suspicion = Configured();
    CHECK_FALSE(suspicion.Add(-1, Points(DetectionKind::Aimbot, 1.0f), Now).has_value());
    CHECK_FALSE(suspicion.Add(MaxSlots, Points(DetectionKind::Aimbot, 1.0f), Now).has_value());
    CHECK(suspicion.Value(-1, DetectionKind::Aimbot, Now) == doctest::Approx(0.0f));
    CHECK(suspicion.Total(MaxSlots, Now) == doctest::Approx(0.0f));
}

TEST_CASE("The top contributor names the rule carrying the most weight")
{
    Suspicion suspicion = Configured();
    suspicion.Add(Slot, Points(DetectionKind::Aimbot, 2.0f), Now);
    suspicion.Add(Slot, Points(DetectionKind::Wallhack, 5.0f), Now);
    CHECK(suspicion.TopContributor(Slot, Now) == DetectionKind::Wallhack);
}
