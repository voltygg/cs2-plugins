#include "Detect/Suspicion.hpp"

#include <cmath>
#include <format>

namespace Anticheat
{

/** Rules below this share of one unit are noise in a breakdown line. */
static constexpr float BreakdownFloor = 0.05f;

std::vector<std::string_view> Suspicion::Configure(const SuspicionBands& bands)
{
    std::vector<std::string_view> rejected;
    _bands = bands;

    // Out of order, the bands would stick at a level the score can never leave.
    if (!std::isfinite(_bands.Suspect) || _bands.Suspect <= 0.0f || _bands.Likely <= _bands.Suspect ||
        _bands.Certain <= _bands.Likely)
    {
        const SuspicionBands defaults;
        _bands.Suspect = defaults.Suspect;
        _bands.Likely = defaults.Likely;
        _bands.Certain = defaults.Certain;
        rejected.push_back("bands");
    }
    if (!std::isfinite(_bands.ReportAgainBelow) || _bands.ReportAgainBelow < 0.0f || _bands.ReportAgainBelow >= 1.0f)
    {
        _bands.ReportAgainBelow = SuspicionBands{}.ReportAgainBelow;
        rejected.push_back("reportAgainBelow");
    }
    return rejected;
}

float Suspicion::Threshold(Confidence level) const
{
    switch (level)
    {
    case Confidence::Suspect:
        return _bands.Suspect;
    case Confidence::Likely:
        return _bands.Likely;
    case Confidence::Certain:
        return _bands.Certain;
    }
    return _bands.Suspect;
}

std::optional<Confidence> Suspicion::BandOf(float total) const
{
    if (total >= _bands.Certain)
        return Confidence::Certain;
    if (total >= _bands.Likely)
        return Confidence::Likely;
    if (total >= _bands.Suspect)
        return Confidence::Suspect;
    return std::nullopt;
}

float Suspicion::Value(int slot, DetectionKind kind, double nowSec) const
{
    if (!InSlotRange(slot) || kind == DetectionKind::Count)
        return 0.0f;
    return static_cast<float>(_slots[slot].Scores[static_cast<size_t>(kind)].Value(nowSec));
}

float Suspicion::Total(int slot, double nowSec) const
{
    if (!InSlotRange(slot))
        return 0.0f;
    float total = 0.0f;
    for (size_t index = 0; index < DetectionKindCount; ++index)
        total += Value(slot, static_cast<DetectionKind>(index), nowSec);
    return total;
}

DetectionKind Suspicion::TopContributor(int slot, double nowSec) const
{
    DetectionKind top = DetectionKind::Aimbot;
    float best = -1.0f;
    for (size_t index = 0; index < DetectionKindCount; ++index)
    {
        const auto kind = static_cast<DetectionKind>(index);
        const float share = Value(slot, kind, nowSec);
        if (share > best)
        {
            best = share;
            top = kind;
        }
    }
    return top;
}

std::string Suspicion::Breakdown(int slot, double nowSec) const
{
    std::string text;
    for (size_t index = 0; index < DetectionKindCount; ++index)
    {
        const auto kind = static_cast<DetectionKind>(index);
        const float share = Value(slot, kind, nowSec);
        if (share < BreakdownFloor)
            continue;
        if (!text.empty())
            text += ", ";
        text += std::format("{} {:.2f}", TokenName(kind), share);
    }
    return text;
}

std::optional<Finding> Suspicion::Add(int slot, const Contribution& contribution, double nowSec)
{
    if (!InSlotRange(slot) || contribution.Kind == DetectionKind::Count || !std::isfinite(contribution.Points))
        return std::nullopt;

    SlotState& state = _slots[slot];

    // Decay since the last contribution may have taken the score below the band it last reported.
    // Measured before this contribution lands, or fresh points would mask the very decay we want.
    const float decayed = Total(slot, nowSec);
    while (state.HasReported && decayed < Threshold(state.Reported) * _bands.ReportAgainBelow)
    {
        if (state.Reported == Confidence::Suspect)
            state.HasReported = false;
        else
            state.Reported = static_cast<Confidence>(static_cast<int>(state.Reported) - 1);
    }

    VoltMod::DecayingScore& score = state.Scores[static_cast<size_t>(contribution.Kind)];
    score.SetHalfLife(contribution.HalfLifeSec);
    score.Add(nowSec, contribution.Points);

    const float total = Total(slot, nowSec);
    const std::optional<Confidence> band = BandOf(total);
    if (!band)
        return std::nullopt;

    Confidence level = *band;
    // Fused evidence may alert, but only a rule confident on its own may get someone punished.
    if (level == Confidence::Certain && Value(slot, TopContributor(slot, nowSec), nowSec) < 1.0f)
        level = Confidence::Likely;

    if (state.HasReported && level <= state.Reported)
        return std::nullopt;

    state.Reported = level;
    state.HasReported = true;
    return Finding{
        .Kind = contribution.Kind,
        .Level = level,
        .KickOnly = contribution.KickOnly,
        .Suspicion = total,
        .Evidence = std::format("{} Suspicion {:.2f} ({}).", contribution.Reason, total, Breakdown(slot, nowSec)),
    };
}

void Suspicion::OnSlotChanged(int slot)
{
    if (InSlotRange(slot))
        _slots[slot] = {};
}

void Suspicion::Reset()
{
    _slots = {};
}

}  // namespace Anticheat
