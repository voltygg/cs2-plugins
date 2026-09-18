#include "Detect/Suspicion.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <numeric>

namespace Anticheat
{

/** Rules below this share of one unit are noise in a breakdown line. */
static constexpr float BreakdownFloor = 0.05f;

std::optional<Confidence> Suspicion::BandOf(float total)
{
    for (size_t index = ConfidenceCount; index-- > 0;)
        if (total >= BandThresholds[index])
            return static_cast<Confidence>(index);
    return std::nullopt;
}

Suspicion::Shares Suspicion::Decayed(int slot, double nowSec) const
{
    Shares shares{};
    if (!InSlotRange(slot))
        return shares;
    for (size_t index = 0; index < DetectionKindCount; ++index)
        shares[index] = static_cast<float>(_slots[slot].Scores[index].Value(nowSec));
    return shares;
}

std::string Suspicion::Describe(const Shares& shares)
{
    std::string text;
    for (size_t index = 0; index < DetectionKindCount; ++index)
    {
        if (shares[index] < BreakdownFloor)
            continue;
        if (!text.empty())
            text += ", ";
        text += std::format("{} {:.2f}", TokenName(static_cast<DetectionKind>(index)), shares[index]);
    }
    return text;
}

float Suspicion::Value(int slot, DetectionKind kind, double nowSec) const
{
    if (!InSlotRange(slot) || kind == DetectionKind::Count)
        return 0.0f;
    return static_cast<float>(_slots[slot].Scores[static_cast<size_t>(kind)].Value(nowSec));
}

float Suspicion::Total(int slot, double nowSec) const
{
    return InSlotRange(slot) ? Total(_slots[slot], nowSec) : 0.0f;
}

float Suspicion::Total(const PlayerEvidence& evidence, double nowSec)
{
    float total = 0.0f;
    for (const VoltMod::DecayingScore& score : evidence.Scores)
        total += static_cast<float>(score.Value(nowSec));
    return total;
}

std::string Suspicion::Breakdown(int slot, double nowSec) const
{
    return Describe(Decayed(slot, nowSec));
}

bool Suspicion::Add(int slot, const Contribution& contribution, double nowSec)
{
    if (!InSlotRange(slot) || contribution.Kind == DetectionKind::Count || !std::isfinite(contribution.Points))
        return false;

    PlayerEvidence& state = _slots[slot];

    // Measured before this contribution lands, or fresh points would mask the very decay that
    // re-arms a band.
    if (state.Reported)
    {
        const std::optional<Confidence> held = BandOf(Total(slot, nowSec) / ReportAgainBelow);
        if (!held || *held < *state.Reported)
            state.Reported = held;
    }

    VoltMod::DecayingScore& score = state.Scores[static_cast<size_t>(contribution.Kind)];
    score.SetHalfLife(contribution.HalfLifeSec);
    score.Add(nowSec, contribution.Points);

    const Shares shares = Decayed(slot, nowSec);
    const float total = std::accumulate(shares.begin(), shares.end(), 0.0f);
    const std::optional<Confidence> band = BandOf(total);
    if (!band)
        return false;

    Confidence level = *band;
    // Fused evidence may alert, but only a rule confident on its own may get someone punished.
    if (level == Confidence::Certain && *std::max_element(shares.begin(), shares.end()) < 1.0f)
        level = Confidence::Likely;

    if (state.Reported && level <= *state.Reported)
        return false;

    state.Reported = level;
    if (!_report)
        return true;

    _report(slot,
            Finding{
                .Kind = contribution.Kind,
                .Level = level,
                .KickOnly = contribution.KickOnly,
                .Suspicion = total,
                .Evidence = std::format("{} Suspicion {:.2f} ({}).", contribution.Reason, total, Describe(shares)),
            });
    return true;
}

PlayerEvidence Suspicion::Save(int slot) const
{
    return InSlotRange(slot) ? _slots[slot] : PlayerEvidence{};
}

void Suspicion::Restore(int slot, const PlayerEvidence& evidence)
{
    if (InSlotRange(slot))
        _slots[slot] = evidence;
}

void Suspicion::ClearSlot(int slot)
{
    if (InSlotRange(slot))
        _slots[slot] = {};
}

void Suspicion::Reset()
{
    _slots = {};
}

}  // namespace Anticheat
