// Spin and jitter analysis for AntiAimCore, split out purely to keep both TUs readable. The tuned
// constants live in AntiAimCore.hpp.

#include "AntiAimCore.hpp"
#include "Core/Geometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>

using Anticheat::AntiAimCore;
using Anticheat::AntiAimTuning::DetectionThreshold;
using Anticheat::AntiAimTuning::FastSpinRate;
using Anticheat::AntiAimTuning::FastSpinSeconds;
using Anticheat::AntiAimTuning::JitterTolerance;
using Anticheat::AntiAimTuning::MediumSpinRate;
using Anticheat::AntiAimTuning::MediumSpinSeconds;
using Anticheat::AntiAimTuning::MinimumJitterSpan;
using Anticheat::AntiAimTuning::MinimumSpinRate;
using Anticheat::AntiAimTuning::MotionHistorySize;
using Anticheat::AntiAimTuning::RequiredJitterSeconds;
using Anticheat::AntiAimTuning::SlowSpinSeconds;
using Anticheat::AntiAimTuning::SpinBreakAllowance;
using Anticheat::AntiAimTuning::SpinConsistency;
using Anticheat::AntiAimTuning::SpinSamples;

namespace Anticheat
{

static constexpr float TierRates[] = {MinimumSpinRate, MediumSpinRate, FastSpinRate};
static constexpr float TierSeconds[] = {SlowSpinSeconds, MediumSpinSeconds, FastSpinSeconds};
static_assert(std::size(TierRates) == std::size(TierSeconds));

/** Yaw periods a jitter bind cycles through; each needs four full repetitions to be believed. */
static constexpr int JitterPeriods[] = {2, 3, 5};

void AntiAimCore::EvaluateMotion(SlotData& data, const Command& command, double nowSec, std::optional<Finding>& out)
{
    if (command.ServerTick == data.LastMotionServerTick)
        return;

    const int64_t serverGap =
        data.LastMotionServerTick < 0 ? 1 : static_cast<int64_t>(command.ServerTick) - data.LastMotionServerTick;
    if (serverGap <= 0)
    {
        ResetMotion(data);
        return;
    }
    if (serverGap > static_cast<int64_t>(TickRate))
        ResetMotion(data);
    data.LastMotionServerTick = command.ServerTick;

    // Strictly consecutive server ticks: a lost tick makes any rate computed across it a fiction.
    std::array<const Command*, MotionHistorySize> history{};
    size_t historyCount = 0;
    int64_t wantedTick = command.ServerTick;
    for (auto candidate = data.Commands.rbegin(); candidate != data.Commands.rend() && historyCount < history.size();
         ++candidate)
    {
        if (!candidate->Simulated || !Geometry::IsFinite(candidate->Base) || candidate->ServerTick > wantedTick)
            continue;
        if (candidate->ServerTick < wantedTick)
            break;
        history[historyCount++] = &*candidate;
        --wantedTick;
    }

    bool spinMatches = false;
    float spinRate = 0.0f;
    if (historyCount >= static_cast<size_t>(SpinSamples))
    {
        float total = 0.0f;
        float net = 0.0f;
        for (int i = 0; i < SpinSamples - 1; ++i)
        {
            const float delta = Geometry::YawDelta(history[i + 1]->Base.Yaw, history[i]->Base.Yaw);
            total += std::abs(delta);
            net += delta;
        }
        const float elapsed =
            static_cast<float>(static_cast<int64_t>(history[0]->ServerTick) - history[SpinSamples - 1]->ServerTick) /
            TickRate;
        spinRate = elapsed > 0.0f ? total / elapsed : 0.0f;
        // Direction consistency separates a spinbot from a player flicking back and forth.
        const float consistency = total > 0.0f ? std::abs(net) / total : 0.0f;
        const float latestRate = std::abs(Geometry::YawDelta(history[1]->Base.Yaw, history[0]->Base.Yaw)) * TickRate;
        spinMatches = spinRate >= MinimumSpinRate && latestRate >= MinimumSpinRate && consistency >= SpinConsistency;
    }

    const float commandSeconds = static_cast<float>(serverGap) / TickRate;
    bool spinDetected = false;
    bool spinEpisodeActive = false;
    for (size_t tier = 0; tier < std::size(TierRates); ++tier)
    {
        if (spinMatches && spinRate >= TierRates[tier] && serverGap == 1)
        {
            data.SpinBreakSeconds[tier] = 0.0f;
            if (!data.SuppressContinuous)
                data.SpinSeconds[tier] += 1.0f / TickRate;
            spinEpisodeActive = true;
        }
        else if (data.SpinSeconds[tier] > 0.0f || data.SpinBreakSeconds[tier] > 0.0f)
        {
            // One second of interruption is forgiven, anything longer restarts the episode.
            data.SpinBreakSeconds[tier] += commandSeconds;
            if (data.SpinBreakSeconds[tier] > SpinBreakAllowance)
            {
                data.SpinSeconds[tier] = 0.0f;
                data.SpinBreakSeconds[tier] = 0.0f;
            }
            else
            {
                spinEpisodeActive = true;
            }
        }
        spinDetected = spinDetected || data.SpinSeconds[tier] >= TierSeconds[tier];
    }

    data.SpinActive = spinEpisodeActive;
    if (spinDetected && !data.SuppressContinuous)
    {
        AddEvidence(data, DetectionThreshold, "continuous spin", true, false, nowSec, out);
        data.SpinActive = true;
    }

    int jitterPeriod = 0;
    for (int period : JitterPeriods)
    {
        const int required = period * 4;
        if (historyCount < static_cast<size_t>(required))
            continue;

        bool repeats = true;
        for (int i = 0; i < required - period && repeats; ++i)
            repeats =
                std::abs(Geometry::YawDelta(history[i + period]->Base.Yaw, history[i]->Base.Yaw)) <= JitterTolerance;

        float span = 0.0f;
        for (int i = 0; i < period; ++i)
            for (int j = i + 1; j < period; ++j)
                span = std::max(span, std::abs(Geometry::YawDelta(history[i]->Base.Yaw, history[j]->Base.Yaw)));

        if (repeats && span > MinimumJitterSpan)
        {
            jitterPeriod = period;
            break;
        }
    }

    // Sustain rather than score each repetition: a legitimate 180-degree bind briefly looks the same.
    bool jitterEpisodeActive = false;
    if (jitterPeriod != 0 && serverGap == 1)
    {
        data.JitterBreakSeconds = 0.0f;
        if (!data.SuppressContinuous)
            data.JitterSeconds += 1.0f / TickRate;
        jitterEpisodeActive = true;
    }
    else if (data.JitterSeconds > 0.0f || data.JitterBreakSeconds > 0.0f)
    {
        data.JitterBreakSeconds += commandSeconds;
        if (data.JitterBreakSeconds > SpinBreakAllowance)
        {
            data.JitterSeconds = 0.0f;
            data.JitterBreakSeconds = 0.0f;
        }
        else
        {
            jitterEpisodeActive = true;
        }
    }

    data.JitterActive = jitterEpisodeActive;
    if (data.JitterSeconds >= RequiredJitterSeconds && !data.SuppressContinuous)
    {
        AddEvidence(data, DetectionThreshold, "continuous repeating jitter", true, false, nowSec, out);
        data.JitterActive = true;
    }
}

}  // namespace Anticheat
