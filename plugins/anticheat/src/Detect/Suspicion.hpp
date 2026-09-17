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

/** What one rule's points are worth and how long they last. */
struct KindTuning
{
    bool Enabled = true;
    /** Points of this kind alone that mean "confident": one whole unit of suspicion. */
    float ConfidentAlone = 1.0f;
    /** Seconds for a point to lose half its weight. 0 or less never decays, so a rule can report once and keep its weight. */
    float HalfLifeSec = 600.0f;
};

struct SuspicionTuning
{
    std::array<KindTuning, DetectionKindCount> Kinds{};
    float Suspect = 1.0f;
    float Likely = 2.0f;
    float Certain = 3.0f;
    /** A band may report again once suspicion falls below this fraction of that band. */
    float ReportAgainBelow = 0.75f;
};

/** One rule's evidence. Points are in that rule's own units; Suspicion normalizes them. */
struct Contribution
{
    DetectionKind Kind = DetectionKind::Aimbot;
    float Points = 1.0f;
    bool KickOnly = false;
    /** One clause naming what happened; Suspicion appends the running total. */
    std::string Reason;
};

/**
 * The compiled calibration: each rule's ConfidentAlone is the evidence it reports at on its own,
 * so one rule reaching its own threshold is exactly 1.0 suspicion and today's sensitivity stands.
 */
SuspicionTuning DefaultTuning();

/**
 * One decaying score per player and rule, and the single place that decides whether the evidence
 * so far is worth reporting.
 *
 * Suspicion is the sum of each rule's score over its own ConfidentAlone, so 1.0 means "one rule is
 * confident on its own, or several are most of the way there". That fusion is the point: rules
 * that each stay under their own threshold still add up.
 *
 * Reporting is banded and each band fires once. Nothing is ever cleared on report, so a detection
 * cannot hand a cheat a clean slate; a band speaks again only after the score decays back below
 * @ref SuspicionTuning::ReportAgainBelow of that band.
 */
class Suspicion
{
public:
    /** Replaces the tuning, clamping unusable values, and names the keys it had to fall back on. */
    std::vector<std::string_view> Configure(const SuspicionTuning& tuning);

    /** Record @p contribution; returns a Finding when it crossed a band not yet reported. */
    std::optional<Finding> Add(int slot, const Contribution& contribution, double nowSec);

    /** This rule's decayed points, in the rule's own units. */
    float Value(int slot, DetectionKind kind, double nowSec) const;

    /** Sum over rules of Value/ConfidentAlone - the whole player. The fusion point. */
    float Total(int slot, double nowSec) const;

    /** The rule carrying the most normalized weight, for naming a fused finding. */
    DetectionKind TopContributor(int slot, double nowSec) const;

    /** "aimbot 0.75, wallhack 0.83" - every rule with weight to speak of. */
    std::string Breakdown(int slot, double nowSec) const;

    void OnSlotChanged(int slot);
    /** Map changes do not call this; only an operator reset does. */
    void Reset();

private:
    /** The band @p total falls in, or nothing while it is below Suspect. */
    std::optional<Confidence> BandOf(float total) const;
    float Threshold(Confidence level) const;
    float Normalized(int slot, DetectionKind kind, double nowSec) const;

    struct SlotState
    {
        std::array<VoltMod::DecayingScore, DetectionKindCount> Scores{};
        Confidence Reported = Confidence::Suspect;
        bool HasReported = false;
    };

    std::array<SlotState, MaxSlots> _slots{};
    SuspicionTuning _tuning;
};

}  // namespace Anticheat
