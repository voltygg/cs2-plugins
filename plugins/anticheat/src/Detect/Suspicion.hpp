#pragma once

#include "Detect/Finding.hpp"
#include "Detect/Samples.hpp"

#include <VoltMod/Core/DecayingScore.hpp>
#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Anticheat
{

inline constexpr size_t DetectionKindCount = static_cast<size_t>(DetectionKind::Count);

/** How long evidence takes to lose half its weight. Rules say which one they mean. */
inline constexpr float FadesOverMinutes = 600.0f;
/** For evidence arriving at command rate, where the fade is a rate limit rather than a memory. */
inline constexpr float FadesOverSeconds = 30.0f;
/** For a confirmed fact about the client, which stays true for as long as they are connected. */
inline constexpr float FadesOverTheSession = 3600.0f;

/**
 * One rule's evidence, already on the shared scale.
 *
 * Points is a share of one whole unit: 1.0 is a rule confident on its own, so a rule that reports
 * after four incidents contributes a quarter each time. Keeping that division in the rule is what
 * lets the score stay free of a per-rule table.
 */
struct Contribution
{
    DetectionKind Kind = DetectionKind::Aimbot;
    float Points = 1.0f;
    float HalfLifeSec = FadesOverMinutes;
    bool KickOnly = false;
    /** One clause naming what happened; Suspicion appends the running total. */
    std::string Reason;
};

/** Where the response ladder steps, and when a step may speak again. */
struct SuspicionBands
{
    float Suspect = 1.0f;
    float Likely = 2.0f;
    float Certain = 3.0f;
    /** A band may report again once suspicion falls below this fraction of that band. */
    float ReportAgainBelow = 0.75f;
};

/** One player's accumulated evidence, as carried across a reconnect. */
struct PlayerEvidence
{
    std::array<VoltMod::DecayingScore, DetectionKindCount> Scores{};
    Confidence Reported = Confidence::Suspect;
    bool HasReported = false;
};

/**
 * One decaying score per player and rule, and the single place that decides whether the evidence
 * so far is worth reporting.
 *
 * Suspicion is the sum of every rule's score, so 1.0 means "one rule is confident on its own, or
 * several are most of the way there". That fusion is the point: rules that each stay under their
 * own threshold still add up.
 *
 * Reporting is banded and each band fires once. Nothing is ever cleared on report, so a detection
 * cannot hand a cheat a clean slate; a band speaks again only after the score decays back below
 * @ref SuspicionBands::ReportAgainBelow of that band.
 */
class Suspicion
{
public:
    /** Replaces the bands, clamping unusable values, and names the keys it had to fall back on. */
    std::vector<std::string_view> Configure(const SuspicionBands& bands);

    /** Record @p contribution; returns a Finding when it crossed a band not yet reported. */
    std::optional<Finding> Add(int slot, const Contribution& contribution, double nowSec);

    /** This rule's decayed share of one whole unit. */
    float Value(int slot, DetectionKind kind, double nowSec) const;

    /** Sum over rules - the whole player. The fusion point. */
    float Total(int slot, double nowSec) const;

    /** The rule carrying the most weight, for naming a fused finding. */
    DetectionKind TopContributor(int slot, double nowSec) const;

    /** "aimbot 0.75, wallhack 0.83" - every rule with weight to speak of. */
    std::string Breakdown(int slot, double nowSec) const;

    /** What @p slot has accumulated, so a disconnect need not throw it away. */
    PlayerEvidence Save(int slot) const;
    /** Hand a returning player back what they left with. */
    void Restore(int slot, const PlayerEvidence& evidence);

    void OnSlotChanged(int slot);
    /** Map changes do not call this; only an operator reset does. */
    void Reset();

private:
    /** The band @p total falls in, or nothing while it is below Suspect. */
    std::optional<Confidence> BandOf(float total) const;
    float Threshold(Confidence level) const;

    std::array<PlayerEvidence, MaxSlots> _slots{};
    SuspicionBands _bands;
};

}  // namespace Anticheat
