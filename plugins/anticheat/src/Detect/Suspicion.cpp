#include "Detect/Suspicion.hpp"

#include <cmath>
#include <format>

namespace Anticheat
{

/** Rules below this share of their own threshold are noise in a breakdown line. */
static constexpr float BreakdownFloor = 0.05f;

static bool Usable(float value)
{
    return std::isfinite(value) && value > 0.0f;
}

SuspicionTuning DefaultTuning()
{
    SuspicionTuning tuning;
    const auto set = [&tuning](DetectionKind kind, float confidentAlone, float halfLifeSec) {
        tuning.Kinds[static_cast<size_t>(kind)] = {
            .Enabled = true, .ConfidentAlone = confidentAlone, .HalfLifeSec = halfLifeSec};
    };

    set(DetectionKind::Aimbot, 4.0f, 600.0f);
    set(DetectionKind::Aimlock, 3.0f, 600.0f);
    // AntiAim evidence arrives at command rate, so its decay is a rate limiter, not a memory horizon.
    set(DetectionKind::AntiAim, 100.0f, 30.0f);
    set(DetectionKind::SilentAim, 12.0f, 600.0f);
    set(DetectionKind::Triggerbot, 8.0f, 600.0f);
    set(DetectionKind::Recoil, 3.0f, 600.0f);
    set(DetectionKind::AimAssist, 6.0f, 600.0f);
    set(DetectionKind::Wallhack, 6.0f, 600.0f);
    // Client integrity is a single confirmed fact, and one that stays true for the session.
    set(DetectionKind::DllInjection, 1.0f, 3600.0f);
    set(DetectionKind::InvalidCvar, 1.0f, 3600.0f);
    set(DetectionKind::Namechanger, 1.0f, 600.0f);
    return tuning;
}

std::vector<std::string_view> Suspicion::Configure(const SuspicionTuning& tuning)
{
    std::vector<std::string_view> rejected;
    _tuning = tuning;

    for (size_t index = 0; index < DetectionKindCount; ++index)
    {
        KindTuning& kind = _tuning.Kinds[index];
        // A ConfidentAlone of zero would divide every score by nothing and report everyone.
        if (!Usable(kind.ConfidentAlone))
        {
            kind.ConfidentAlone = KindTuning{}.ConfidentAlone;
            rejected.push_back(TokenName(static_cast<DetectionKind>(index)));
        }
        if (!std::isfinite(kind.HalfLifeSec))
            kind.HalfLifeSec = KindTuning{}.HalfLifeSec;
    }

    // Out of order, the bands would stick at a level the score can never leave.
    if (!Usable(_tuning.Suspect) || _tuning.Likely <= _tuning.Suspect || _tuning.Certain <= _tuning.Likely)
    {
        const SuspicionTuning defaults;
        _tuning.Suspect = defaults.Suspect;
        _tuning.Likely = defaults.Likely;
        _tuning.Certain = defaults.Certain;
        rejected.push_back("bands");
    }
    if (!std::isfinite(_tuning.ReportAgainBelow) || _tuning.ReportAgainBelow < 0.0f ||
        _tuning.ReportAgainBelow >= 1.0f)
    {
        _tuning.ReportAgainBelow = SuspicionTuning{}.ReportAgainBelow;
        rejected.push_back("reportAgainBelow");
    }
    return rejected;
}

float Suspicion::Threshold(Confidence level) const
{
    switch (level)
    {
    case Confidence::Suspect:
        return _tuning.Suspect;
    case Confidence::Likely:
        return _tuning.Likely;
    case Confidence::Certain:
        return _tuning.Certain;
    }
    return _tuning.Suspect;
}

std::optional<Confidence> Suspicion::BandOf(float total) const
{
    if (total >= _tuning.Certain)
        return Confidence::Certain;
    if (total >= _tuning.Likely)
        return Confidence::Likely;
    if (total >= _tuning.Suspect)
        return Confidence::Suspect;
    return std::nullopt;
}

float Suspicion::Value(int slot, DetectionKind kind, double nowSec) const
{
    if (!InSlotRange(slot) || kind == DetectionKind::Count)
        return 0.0f;
    return static_cast<float>(_slots[slot].Scores[static_cast<size_t>(kind)].Value(nowSec));
}

float Suspicion::Normalized(int slot, DetectionKind kind, double nowSec) const
{
    const KindTuning& tuning = _tuning.Kinds[static_cast<size_t>(kind)];
    if (!tuning.Enabled)
        return 0.0f;
    return Value(slot, kind, nowSec) / tuning.ConfidentAlone;
}

float Suspicion::Total(int slot, double nowSec) const
{
    if (!InSlotRange(slot))
        return 0.0f;
    float total = 0.0f;
    for (size_t index = 0; index < DetectionKindCount; ++index)
        total += Normalized(slot, static_cast<DetectionKind>(index), nowSec);
    return total;
}

DetectionKind Suspicion::TopContributor(int slot, double nowSec) const
{
    DetectionKind top = DetectionKind::Aimbot;
    float best = -1.0f;
    for (size_t index = 0; index < DetectionKindCount; ++index)
    {
        const auto kind = static_cast<DetectionKind>(index);
        const float share = Normalized(slot, kind, nowSec);
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
        const float share = Normalized(slot, kind, nowSec);
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
    if (!InSlotRange(slot) || contribution.Kind == DetectionKind::Count)
        return std::nullopt;

    const size_t index = static_cast<size_t>(contribution.Kind);
    const KindTuning& tuning = _tuning.Kinds[index];
    if (!tuning.Enabled || !std::isfinite(contribution.Points))
        return std::nullopt;

    SlotState& state = _slots[slot];

    // Decay since the last contribution may have taken the score below the band it last reported.
    // Measured before this contribution lands, or fresh points would mask the very decay we want.
    const float decayed = Total(slot, nowSec);
    while (state.HasReported && decayed < Threshold(state.Reported) * _tuning.ReportAgainBelow)
    {
        if (state.Reported == Confidence::Suspect)
            state.HasReported = false;
        else
            state.Reported = static_cast<Confidence>(static_cast<int>(state.Reported) - 1);
    }

    state.Scores[index].SetHalfLife(tuning.HalfLifeSec);
    state.Scores[index].Add(nowSec, contribution.Points);

    const float total = Total(slot, nowSec);
    const std::optional<Confidence> band = BandOf(total);
    if (!band)
        return std::nullopt;

    Confidence level = *band;
    const DetectionKind top = TopContributor(slot, nowSec);
    // Fused evidence may alert, but only a rule confident on its own may get someone punished.
    if (level == Confidence::Certain && Normalized(slot, top, nowSec) < 1.0f)
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
