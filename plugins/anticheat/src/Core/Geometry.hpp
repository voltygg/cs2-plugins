#pragma once

// Angle math shared by every aim core. Deliberately NOT the kit's AngleMath: that measures
// pitch/yaw error in a Euclidean plane, which under-reports near the poles and over-reports at
// large yaw offsets. Aim detection compares directions, so distances here are great-circle
// (acos of the dot product of the two forward vectors), matching what a bullet actually does.

#include "Samples.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace Anticheat::Geometry
{

inline constexpr double DegreesPerRadian = 180.0 / std::numbers::pi;
inline constexpr double RadiansPerDegree = std::numbers::pi / 180.0;

/** Head, chest and feet-ish sample points an aimbot may lock onto. */
inline constexpr float BodyHeights[] = {8.0f, 46.0f, 64.0f};
inline constexpr int BodyPointCount = static_cast<int>(std::size(BodyHeights));

inline bool IsFinite(float value)
{
    return std::isfinite(value);
}

inline bool IsFinite(const Vec3& v)
{
    return std::isfinite(v.X) && std::isfinite(v.Y) && std::isfinite(v.Z);
}

inline bool IsFinite(const AimAngles& a)
{
    return std::isfinite(a.Pitch) && std::isfinite(a.Yaw);
}

/** Unit forward vector for @p angles (CS2 pitch is inverted: positive pitch looks down). */
inline Vec3 AimForward(const AimAngles& angles)
{
    const float pitch = static_cast<float>(angles.Pitch * RadiansPerDegree);
    const float yaw = static_cast<float>(angles.Yaw * RadiansPerDegree);
    const float cosPitch = std::cos(pitch);
    return {cosPitch * std::cos(yaw), cosPitch * std::sin(yaw), -std::sin(pitch)};
}

inline float Dot(const Vec3& a, const Vec3& b)
{
    return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
}

/** Great-circle angle between two aim directions, in degrees. NaN when either side is not finite. */
inline float AngularDistance(const AimAngles& first, const AimAngles& second)
{
    if (!IsFinite(first) || !IsFinite(second))
        return std::numeric_limits<float>::quiet_NaN();
    const float dot = std::clamp(Dot(AimForward(first), AimForward(second)), -1.0f, 1.0f);
    return static_cast<float>(std::acos(dot) * DegreesPerRadian);
}

/** Half-angle a feature of half-width @p halfWidth subtends at @p distance, in degrees. */
inline float AngularSizeDeg(float halfWidth, float distance)
{
    return static_cast<float>(std::atan2(halfWidth, distance) * DegreesPerRadian);
}

/** Angle between where @p angles points from @p eye and the direction to @p target. */
inline float AimErrorDeg(const Vec3& eye, const AimAngles& angles, const Vec3& target)
{
    Vec3 direction = target - eye;
    if (!IsFinite(direction) || direction.LengthSqr() < 1e-6f)
        return 0.0f;
    const float length = direction.Length();
    direction = {direction.X / length, direction.Y / length, direction.Z / length};
    const float dot = std::clamp(Dot(AimForward(angles), direction), -1.0f, 1.0f);
    return static_cast<float>(std::acos(dot) * DegreesPerRadian);
}

/** Smallest aim error against the three body points above @p feet. */
inline float NearestBodyAimError(const Vec3& eye, const AimAngles& angles, const Vec3& feet)
{
    float best = 180.0f;
    for (float height : BodyHeights)
        best = std::min(best, AimErrorDeg(eye, angles, {feet.X, feet.Y, feet.Z + height}));
    return best;
}

/** Angles pointing from @p eye at @p target. */
inline AimAngles Bearing(const Vec3& eye, const Vec3& target)
{
    const Vec3 delta = target - eye;
    const float horizontal = std::sqrt(delta.X * delta.X + delta.Y * delta.Y);
    return {static_cast<float>(-std::atan2(delta.Z, horizontal) * DegreesPerRadian),
            static_cast<float>(std::atan2(delta.Y, delta.X) * DegreesPerRadian)};
}

/** Shortest signed yaw travel from @p from to @p to, in (-180, 180]. */
inline float YawDelta(float from, float to)
{
    return std::remainder(to - from, 360.0f);
}

}  // namespace Anticheat::Geometry
