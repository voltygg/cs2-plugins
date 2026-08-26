#include "Core/WeaponClass.hpp"
#include "Detectors/SilentAimCore.hpp"

#include <doctest/doctest.h>
#include <string>

using Anticheat::DetectionKind;
using Anticheat::Finding;
using Anticheat::IsBallisticWeapon;
using Anticheat::NormalizeWeapon;
using Anticheat::ShotView;
using Anticheat::SilentAimCore;
using Anticheat::SilentAimDeviationThreshold;
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

static std::optional<Finding> Land(SilentAimCore& core, ShotView shot, double now = Now)
{
    core.OnShotUpdated(Slot, shot);
    return core.Finalize(Slot, shot, now);
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

TEST_CASE("A deviation the weapon can explain scores nothing")
{
    SilentAimCore core;
    // 5.71 degrees is well inside a rifle's 12.5 degree ceiling.
    CHECK_FALSE(Land(core, Shot("ak47", ModerateImpact)).has_value());
    CHECK(core.Score(Slot, Now) == 0);
}

TEST_CASE("The same deviation on a sniper rifle is beyond explanation and scores")
{
    SilentAimCore core;
    CHECK_FALSE(Land(core, Shot("awp", ModerateImpact)).has_value());
    CHECK(core.Score(Slot, Now) == 2);
}

TEST_CASE("An impact closer than a hundred units or beyond ten thousand is not measured")
{
    SilentAimCore near;
    Land(near, Shot("awp", {50.0f, 50.0f, 0.0f}));
    CHECK(near.Score(Slot, Now) == 0);

    SilentAimCore far;
    Land(far, Shot("awp", {15000.0f, 1500.0f, 0.0f}));
    CHECK(far.Score(Slot, Now) == 0);
}

TEST_CASE("A shot is only finalized into evidence when it both hurt someone and reported an impact")
{
    SilentAimCore missed;
    Land(missed, Shot("awp", ModerateImpact, false));
    CHECK(missed.Score(Slot, Now) == 0);

    SilentAimCore unseen;
    ShotView shot = Shot("awp", ModerateImpact);
    shot.ImpactSeen = false;
    unseen.OnShotUpdated(Slot, shot);
    unseen.Finalize(Slot, shot, Now);
    CHECK(unseen.Score(Slot, Now) == 0);
}

TEST_CASE("The points formula weighs blatant deviation airborne shots headshots and wallbangs")
{
    SilentAimCore grounded;
    Land(grounded, Shot("awp", ModerateImpact));
    CHECK(grounded.Score(Slot, Now) == 2);

    SilentAimCore airborne;
    Land(airborne, Shot("awp", ModerateImpact, true, true));
    CHECK(airborne.Score(Slot, Now) == 1);

    SilentAimCore blatant;
    Land(blatant, Shot("awp", BlatantImpact));
    CHECK(blatant.Score(Slot, Now) == 3);

    SilentAimCore decorated;
    Land(decorated, Shot("awp", ModerateImpact, true, false, true, true));
    CHECK(decorated.Score(Slot, Now) == 4);

    SilentAimCore worst;
    Land(worst, Shot("awp", BlatantImpact, true, false, true, true));
    CHECK(worst.Score(Slot, Now) == 5);
}

TEST_CASE("The rolling score fires at twelve and not at ten")
{
    SilentAimCore core;
    for (int i = 0; i < 5; ++i)
        CHECK_FALSE(Land(core, Shot("awp", ModerateImpact)).has_value());
    CHECK(core.Score(Slot, Now) == 10);

    const std::optional<Finding> finding = Land(core, Shot("awp", ModerateImpact));
    REQUIRE(finding.has_value());
    CHECK(finding->Kind == DetectionKind::SilentAim);
    CHECK_FALSE(finding->KickOnly);
    CHECK(core.Score(Slot, Now) == 0);  // firing clears the window
}

TEST_CASE("Incidents older than ten minutes stop counting toward the score")
{
    SilentAimCore core;
    for (int i = 0; i < 5; ++i)
        Land(core, Shot("awp", ModerateImpact));
    CHECK(core.Score(Slot, Now) == 10);

    // Eleven minutes later the old points are gone, so a new blatant hit cannot reach twelve.
    CHECK_FALSE(Land(core, Shot("awp", BlatantImpact), Now + 660.0).has_value());
    CHECK(core.Score(Slot, Now + 660.0) == 3);
}

TEST_CASE("A slot change drops the slot's accumulated silent aim evidence")
{
    SilentAimCore core;
    for (int i = 0; i < 5; ++i)
        Land(core, Shot("awp", ModerateImpact));
    core.OnSlotChanged(Slot);
    CHECK(core.Score(Slot, Now) == 0);
}
