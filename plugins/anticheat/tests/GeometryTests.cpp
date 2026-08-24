#include "Core/Geometry.hpp"

#include <cmath>
#include <doctest/doctest.h>

using namespace Anticheat;
using namespace Anticheat::Geometry;

namespace
{
bool Near(float a, float b, float eps = 0.01f)
{
    return std::fabs(a - b) < eps;
}
}  // namespace

TEST_CASE("AimForward points down positive X at zero angles and straight down at 90 degrees pitch")
{
    const Vec3 level = AimForward({0.0f, 0.0f});
    CHECK(Near(level.X, 1.0f));
    CHECK(Near(level.Y, 0.0f));
    CHECK(Near(level.Z, 0.0f));

    const Vec3 down = AimForward({90.0f, 0.0f});
    CHECK(Near(down.X, 0.0f));
    CHECK(Near(down.Z, -1.0f));

    const Vec3 east = AimForward({0.0f, 90.0f});
    CHECK(Near(east.X, 0.0f));
    CHECK(Near(east.Y, 1.0f));
}

TEST_CASE("AngularDistance equals the yaw difference when both angles are level")
{
    CHECK(Near(AngularDistance({0.0f, 0.0f}, {0.0f, 30.0f}), 30.0f));
    CHECK(Near(AngularDistance({0.0f, 170.0f}, {0.0f, -170.0f}), 20.0f));
}

// Euclidean pitch/yaw distance gives 90 degrees here. Great-circle distance
// gives 14 degrees, which is why the aim cores use it.
TEST_CASE("AngularDistance is great-circle and diverges from a Euclidean pitch-yaw metric near the poles")
{
    const float greatCircle = AngularDistance({80.0f, 0.0f}, {80.0f, 90.0f});
    CHECK(Near(greatCircle, 14.10f, 0.05f));
    CHECK(greatCircle < 90.0f);
}

TEST_CASE("AngularDistance is not a number when either side is not finite")
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    CHECK(std::isnan(AngularDistance({nan, 0.0f}, {0.0f, 0.0f})));
    CHECK(std::isnan(AngularDistance({0.0f, 0.0f}, {0.0f, nan})));
}

TEST_CASE("AngularSizeDeg matches the arctangent of the half width over the distance")
{
    CHECK(Near(AngularSizeDeg(16.0f, 1000.0f), 0.9167f, 0.001f));
    CHECK(Near(AngularSizeDeg(16.0f, 500.0f), 1.8331f, 0.001f));
    CHECK(Near(AngularSizeDeg(48.0f, 500.0f), 5.4834f, 0.001f));
}

TEST_CASE("NearestBodyAimError picks the closest of the three body points")
{
    const Vec3 eye{0.0f, 0.0f, 64.0f};
    const Vec3 feet{500.0f, 0.0f, 0.0f};
    // The 64-unit body point is level with the eye, so a level aim of y degrees is off by exactly y.
    CHECK(Near(NearestBodyAimError(eye, {0.0f, 0.0f}, feet), 0.0f));
    CHECK(Near(NearestBodyAimError(eye, {0.0f, 5.0f}, feet), 5.0f));
    CHECK(Near(NearestBodyAimError(eye, {0.0f, -12.0f}, feet), 12.0f));
}

TEST_CASE("Bearing points at the target and inverts pitch the way the engine does")
{
    const AimAngles east = Bearing({0.0f, 0.0f, 0.0f}, {100.0f, 0.0f, 0.0f});
    CHECK(Near(east.Pitch, 0.0f));
    CHECK(Near(east.Yaw, 0.0f));

    const AimAngles north = Bearing({0.0f, 0.0f, 0.0f}, {0.0f, 100.0f, 0.0f});
    CHECK(Near(north.Yaw, 90.0f));

    const AimAngles above = Bearing({0.0f, 0.0f, 0.0f}, {100.0f, 0.0f, 100.0f});
    CHECK(Near(above.Pitch, -45.0f));
}

TEST_CASE("YawDelta takes the short way around the circle")
{
    CHECK(Near(YawDelta(170.0f, -170.0f), 20.0f));
    CHECK(Near(YawDelta(-170.0f, 170.0f), -20.0f));
    CHECK(Near(YawDelta(0.0f, 45.0f), 45.0f));
}

TEST_CASE("IsFinite rejects infinities and not-a-number in vectors and angles")
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    CHECK(IsFinite(Vec3{1.0f, 2.0f, 3.0f}));
    CHECK_FALSE(IsFinite(Vec3{1.0f, nan, 3.0f}));
    CHECK_FALSE(IsFinite(Vec3{inf, 0.0f, 0.0f}));
    CHECK(IsFinite(AimAngles{10.0f, 20.0f}));
    CHECK_FALSE(IsFinite(AimAngles{10.0f, nan}));
}
