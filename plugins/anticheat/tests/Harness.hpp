#pragma once

#include "Detect/Finding.hpp"
#include "Detect/Samples.hpp"
#include "Detect/ShotHistory.hpp"
#include "Detect/Suspicion.hpp"

#include <array>
#include <optional>

namespace Anticheat::Test
{

/** The two slots every rule scenario uses: one watching, one being watched. */
inline constexpr int Observer = 0;
inline constexpr int Target = 1;
inline constexpr double Now = 100.0;
inline constexpr Vec3 Eye{0.0f, 0.0f, 64.0f};
inline constexpr float TargetX = 500.0f;

/** What a rule reported, in place of the findings rules used to return. */
struct Findings
{
    int Count = 0;
    std::optional<Finding> Last;

    /** Wire into a Suspicion with `Scores.ReportTo(findings.Sink())`. */
    auto Sink()
    {
        return [this](int, const Finding& finding) {
            ++Count;
            Last = finding;
        };
    }

    void Clear()
    {
        Count = 0;
        Last.reset();
    }
};

/** How a player stands in one frame. Defaults describe an ordinary alive opponent. */
struct Standing
{
    Vec3 Origin;
    int Team = TeamCT;
    bool Alive = true;
    bool Teleported = false;
    /** Which viewers have a traced sight line, and which of them can see through it. */
    uint64_t CheckedBy = 0;
    uint64_t SeenBy = 0;
};

inline PositionSample Sample(const Standing& standing)
{
    return {.Origin = standing.Origin,
            .EyePos = {standing.Origin.X, standing.Origin.Y, standing.Origin.Z + 64.0f},
            .Team = standing.Team,
            .Valid = true,
            .Alive = standing.Alive,
            .Teleported = standing.Teleported,
            .CheckedBy = standing.CheckedBy,
            .SeenBy = standing.SeenBy};
}

/** One frame: the observer at the origin on T, and one enemy standing as @p target says. */
inline std::array<PositionSample, MaxSlots> Frame(const Standing& target)
{
    std::array<PositionSample, MaxSlots> players{};
    players[Observer] = Sample({.Origin = {0.0f, 0.0f, 0.0f}, .Team = TeamT});
    players[Target] = Sample(target);
    return players;
}

}  // namespace Anticheat::Test
