#pragma once

#include "Detect/Finding.hpp"
#include "Detect/Samples.hpp"

#include <VoltMod/Core/DecayingScore.hpp>
#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace Anticheat
{

inline constexpr size_t DetectionKindCount = static_cast<size_t>(DetectionKind::Count);
inline constexpr size_t ConfidenceCount = static_cast<size_t>(Confidence::Certain) + 1;

/** How long evidence takes to lose half its weight. Rules say which one they mean. */
inline constexpr float FadesOverMinutes = 600.0f;
/** For evidence arriving at command rate, where the fade is a rate limit rather than a memory. */
inline constexpr float FadesOverSeconds = 30.0f;
/** For a confirmed fact about the client, which stays true for as long as they are connected. */
inline constexpr float FadesOverTheSession = 3600.0f;

/** Where the response ladder steps, in Confidence order. */
inline constexpr std::array<float, ConfidenceCount> BandThresholds{1.0f, 2.0f, 3.0f};
/** A band may speak again once suspicion falls below this fraction of it. */
inline constexpr float ReportAgainBelow = 0.75f;

/** One rule's evidence on the shared scale. Points is a share of one whole unit, so a rule that
 *  reports after four incidents contributes a quarter each time; keeping that division in the rule
 *  is what frees the score of a per-rule table. Reason is read during the call only. */
struct Contribution
{
    DetectionKind Kind = DetectionKind::Aimbot;
    float Points = 1.0f;
    float HalfLifeSec = FadesOverMinutes;
    bool KickOnly = false;
    /** One clause naming what happened; Suspicion appends the running total. */
    std::string_view Reason;
};

/** One player's accumulated evidence, as carried across a reconnect. */
struct PlayerEvidence
{
    std::array<VoltMod::DecayingScore, DetectionKindCount> Scores{};
    /** The highest band already reported, while it still holds. */
    std::optional<Confidence> Reported;
};

/**
 * One decaying score per player and rule, and the only place a report is decided. The total sums
 * over rules, so rules under their own thresholds still add up. Nothing is cleared on report; each
 * band fires once and speaks again only below @ref ReportAgainBelow of it.
 */
class Suspicion
{
public:
    /** Where a finding goes once the evidence produces one. */
    using Reporter = std::function<void(int slot, const Finding& finding)>;

    /** Wire the response in. Unset until then, so a rule under test reports nowhere. */
    void ReportTo(Reporter reporter) { _report = std::move(reporter); }

    /** Record @p contribution. Reports through @ref ReportTo and returns true when it crossed a
     *  band not yet reported, which is how a rule knows to latch its episode. */
    bool Add(int slot, const Contribution& contribution, double nowSec);

    /** This rule's decayed share of one whole unit. */
    float Value(int slot, DetectionKind kind, double nowSec) const;

    /** Sum over rules - the whole player. The fusion point. */
    float Total(int slot, double nowSec) const;

    /** The same sum over evidence that is saved rather than live in a slot. */
    static float Total(const PlayerEvidence& evidence, double nowSec);

    /** "aimbot 0.75, wallhack 0.83" - every rule with weight to speak of. */
    std::string Breakdown(int slot, double nowSec) const;

    /** What @p slot has accumulated, so a disconnect need not throw it away. */
    PlayerEvidence Save(int slot) const;
    /** Hand a returning player back what they left with. */
    void Restore(int slot, const PlayerEvidence& evidence);

    void ClearSlot(int slot);
    /** Map changes do not call this; only an operator reset does. */
    void Reset();

private:
    using Shares = std::array<float, DetectionKindCount>;

    /** Every rule's decayed share, so one sweep answers total, largest and breakdown. */
    Shares Decayed(int slot, double nowSec) const;
    /** The band @p total falls in, or nothing while it is below Suspect. */
    static std::optional<Confidence> BandOf(float total);
    static std::string Describe(const Shares& shares);

    std::array<PlayerEvidence, MaxSlots> _slots{};
    Reporter _report;
};

}  // namespace Anticheat
