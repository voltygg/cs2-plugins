#include "Detect/WeaponClass.hpp"
#include "Detect/Rules/SilentAim.hpp"

#include <doctest/doctest.h>
#include <string>

using Anticheat::DetectionKind;
using Anticheat::Finding;
using Anticheat::IsBallisticWeapon;
using Anticheat::NormalizeWeapon;
using Anticheat::ShotView;
using Anticheat::Rules::SilentAim;
using Anticheat::SilentAimDeviationThreshold;
using Anticheat::Suspicion;
using Anticheat::Vec3;

static constexpr int Slot = 0;
static constexpr double Now = 500.0;

/** Eye at the origin looking down positive X, so an impact at (1000, y, 0) deviates by atan(y/1000). */
static ShotView Shot(std::string weapon, Vec3 impact, bool hurt = true, bool airborne = false, bool headshot = false,
                     bool wallbang = false)
{
    ShotView shot;
    shot.Slot = Slot;
    shot.HasVisibleAngles = true;
    shot.VisibleAngles = {0.0f, 0.0f};
    shot.EyePos = {0.0f, 0.0f, 0.0f};
    shot.ImpactPos = impact;
    shot.ImpactSeen = true;
    shot.HurtSeen = hurt;
    shot.Weapon = std::move(weapon);
    shot.Airborne = airborne;
    shot.Headshot = headshot;
    shot.Wallbang = wallbang;
    return shot;
}

/** ~5.71 degrees off: above every precision weapon's ceiling, below the SMG and blatant ones. */
static constexpr Vec3 ModerateImpact{1000.0f, 100.0f, 0.0f};
/** 45 degrees off at 141 units: past the blatant threshold. */
static constexpr Vec3 BlatantImpact{100.0f, 100.0f, 0.0f};

/** The rule and the score it feeds, since a finding now comes out of the score. */
struct SilentAimHarness
{
    SilentAimHarness() { Scores.Configure({}); }

    /** Points this rule has on the slot, in its own units, where twelve weighted points are one whole unit of suspicion, as of @p now. */
    float Points(double now = Now) const { return Scores.Value(Slot, DetectionKind::SilentAim, now) * 12.0f; }

    Suspicion Scores;
    SilentAim Rule{Scores};
};

static std::optional<Finding> Land(SilentAimHarness& h, ShotView shot, double now = Now)
{
    h.Rule.OnShotUpdated(Slot, shot);
    return h.Rule.Finalize(Slot, shot, now);
}

TEST_CASE("The per weapon deviation table matches each weapon class")
{
    CHECK(SilentAimDeviationThreshold("ak47") == doctest::Approx(12.5f));
    CHECK(SilentAimDeviationThreshold("weapon_m4a1_silencer") == doctest::Approx(12.5f));
    CHECK(SilentAimDeviationThreshold("scar20") == doctest::Approx(12.5f));
    CHECK(SilentAimDeviationThreshold("awp") == doctest::Approx(2.5f));
    CHECK(SilentAimDeviationThreshold("ssg08") == doctest::Approx(2.5f));
    CHECK(SilentAimDeviationThreshold("deagle") == doctest::Approx(4.5f));
    CHECK(SilentAimDeviationThreshold("revolver") == doctest::Approx(4.5f));
    CHECK(SilentAimDeviationThreshold("glock") == doctest::Approx(4.3f));
    CHECK(SilentAimDeviationThreshold("cz75a") == doctest::Approx(4.3f));
    CHECK(SilentAimDeviationThreshold("mp9") == doctest::Approx(22.5f));
    CHECK(SilentAimDeviationThreshold("bizon") == doctest::Approx(22.5f));
    CHECK(SilentAimDeviationThreshold("nova") == doctest::Approx(13.5f));
    CHECK(SilentAimDeviationThreshold("negev") == doctest::Approx(13.5f));
    CHECK(SilentAimDeviationThreshold("taser") == doctest::Approx(15.5f));
}

TEST_CASE("NormalizeWeapon strips the weapon prefix and leaves everything else alone")
{
    CHECK(NormalizeWeapon("weapon_ak47") == "ak47");
    CHECK(NormalizeWeapon("ak47") == "ak47");
    CHECK(NormalizeWeapon("") == "");
}

TEST_CASE("IsBallisticWeapon accepts hitscan weapons and rejects grenades and the knife")
{
    CHECK(IsBallisticWeapon("weapon_ak47"));
    CHECK(IsBallisticWeapon("awp"));
    CHECK(IsBallisticWeapon("taser"));
    CHECK_FALSE(IsBallisticWeapon("hegrenade"));
    CHECK_FALSE(IsBallisticWeapon("knife"));
    CHECK_FALSE(IsBallisticWeapon("c4"));
}

TEST_CASE("A deviation scores only when the weapon cannot explain it")
{
    // 5.71 degrees is well inside a rifle's 12.5 degree ceiling and far outside an AWP's 2.5.
    SilentAimHarness rifle;
    CHECK_FALSE(Land(rifle, Shot("ak47", ModerateImpact)).has_value());
    CHECK(rifle.Points() == doctest::Approx(0.0f));

    SilentAimHarness sniper;
    CHECK_FALSE(Land(sniper, Shot("awp", ModerateImpact)).has_value());
    CHECK(sniper.Points() == doctest::Approx(2.0f));
}

TEST_CASE("An impact closer than a hundred units or beyond ten thousand is not measured")
{
    SilentAimHarness near;
    Land(near, Shot("awp", {50.0f, 50.0f, 0.0f}));
    CHECK(near.Points() == doctest::Approx(0.0f));

    SilentAimHarness far;
    Land(far, Shot("awp", {15000.0f, 1500.0f, 0.0f}));
    CHECK(far.Points() == doctest::Approx(0.0f));
}

TEST_CASE("A shot is only finalized into evidence when it both hurt someone and reported an impact")
{
    SilentAimHarness missed;
    Land(missed, Shot("awp", ModerateImpact, false));
    CHECK(missed.Points() == doctest::Approx(0.0f));

    SilentAimHarness unseen;
    ShotView shot = Shot("awp", ModerateImpact);
    shot.ImpactSeen = false;
    unseen.Rule.OnShotUpdated(Slot, shot);
    unseen.Rule.Finalize(Slot, shot, Now);
    CHECK(unseen.Points() == doctest::Approx(0.0f));
}

TEST_CASE("The points formula weighs blatant deviation airborne shots headshots and wallbangs")
{
    // The plain moderate-deviation shot above is worth 2; each modifier moves it from there.
    {
        SilentAimHarness airborne;
        Land(airborne, Shot("awp", ModerateImpact, true, true));
        CHECK(airborne.Points() == doctest::Approx(1.0f));
    }
    {
        SilentAimHarness blatant;
        Land(blatant, Shot("awp", BlatantImpact));
        CHECK(blatant.Points() == doctest::Approx(3.0f));
    }
    {
        SilentAimHarness decorated;
        Land(decorated, Shot("awp", ModerateImpact, true, false, true, true));
        CHECK(decorated.Points() == doctest::Approx(4.0f));
    }
    {
        SilentAimHarness worst;
        Land(worst, Shot("awp", BlatantImpact, true, false, true, true));
        CHECK(worst.Points() == doctest::Approx(5.0f));
    }
}

TEST_CASE("The rolling score fires at twelve and not at ten")
{
    SilentAimHarness rule;
    for (int i = 0; i < 5; ++i)
        CHECK_FALSE(Land(rule, Shot("awp", ModerateImpact)).has_value());
    CHECK(rule.Points() == doctest::Approx(10.0f));

    const std::optional<Finding> finding = Land(rule, Shot("awp", ModerateImpact));
    REQUIRE(finding.has_value());
    CHECK(finding->Kind == DetectionKind::SilentAim);
    CHECK_FALSE(finding->KickOnly);
    // Reporting no longer wipes the score, so the next shot builds on twelve rather than on nothing.
    CHECK(rule.Points() == doctest::Approx(12.0f));
}

TEST_CASE("Old points fade rather than falling off a window edge")
{
    SilentAimHarness rule;
    for (int i = 0; i < 5; ++i)
        Land(rule, Shot("awp", ModerateImpact));
    CHECK(rule.Points() == doctest::Approx(10.0f));

    // Eleven minutes is a little over one half-life, so the ten has faded to under five and a
    // blatant hit on top of it still cannot reach twelve.
    CHECK_FALSE(Land(rule, Shot("awp", BlatantImpact), Now + 660.0).has_value());
    CHECK(rule.Points(Now + 660.0) == doctest::Approx(10.0f * 0.4665f + 3.0f).epsilon(0.01));
}

TEST_CASE("A slot change drops the slot's accumulated silent aim evidence")
{
    SilentAimHarness rule;
    for (int i = 0; i < 5; ++i)
        Land(rule, Shot("awp", ModerateImpact));
    rule.Scores.OnSlotChanged(Slot);
    CHECK(rule.Points() == doctest::Approx(0.0f));
}
