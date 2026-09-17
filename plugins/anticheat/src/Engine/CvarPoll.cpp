#include "Engine/CvarPoll.hpp"


#include <algorithm>
#include <chrono>
#include <string>

namespace Anticheat
{

using VoltMod::Time;
using Rules::CvarRuleTable;
using Rules::CvarsPerPoll;
using Rules::InvalidCvar;
using Rules::PollDelaySec;

/** Poll-loop cadence only; the per-slot deadlines do the real timing. */
static constexpr int64_t PollIntervalMs = 1000;

/** A seed in [1, m-1], the only range std::minstd_rand accepts. */
static std::minstd_rand::result_type Seed()
{
    const auto ticks = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    return static_cast<std::minstd_rand::result_type>(ticks % 2147483646U) + 1;
}

void CvarPoll::Initialize()
{
    if (_pollTimer)
        return;

    _random.seed(Seed());
    _pollTimer = _rt.Scheduler.Repeat(PollIntervalMs, [this] {
        if (!_detectors.Enabled() || !_detectors.RuleEnabled(_detectors.InvalidCvars))
            return;
        const double now = Time::MonotonicSeconds();
        for (int slot = 0; slot < MaxSlots; ++slot)
        {
            SlotState& state = _slots[slot];
            if (!_detectors.IsEligible(slot))
                continue;
            // A map change clears every schedule, and players who ride it out never connect again.
            if (state.NextPoll == 0.0)
            {
                state.NextPoll = now + NextDelaySec();
                continue;
            }
            if (now < state.NextPoll)
                continue;
            state.NextPoll = now + NextDelaySec();
            Poll(slot, state);
        }
    });
}

void CvarPoll::OnFullyConnected(int slot)
{
    if (InSlotRange(slot))
        _slots[slot] = {.NextPoll = Time::MonotonicSeconds() + NextDelaySec()};
}

void CvarPoll::OnSlotChanged(int slot)
{
    if (InSlotRange(slot))
        _slots[slot] = {};
}

void CvarPoll::Reset()
{
    _slots = {};
}

double CvarPoll::PollsIn(int slot, double nowSec) const
{
    if (!InSlotRange(slot) || _slots[slot].NextPoll == 0.0)
        return 0.0;
    return std::max(0.0, _slots[slot].NextPoll - nowSec);
}

double CvarPoll::NextDelaySec()
{
    return PollDelaySec(std::generate_canonical<double, 24>(_random));
}

void CvarPoll::ReportVerdict(int slot, const std::optional<Rules::CvarVerdict>& verdict)
{
    if (!verdict)
        return;
    _detectors.Report(slot, _detectors.Scores.Add(slot,
                                                 {.Kind = Rules::InvalidCvar::Kind,
                                                  .Points = 1.0f,
                                                  .KickOnly = verdict->KickOnly,
                                                  .Reason = verdict->Reason},
                                                 VoltMod::Time::MonotonicSeconds()));
}

void CvarPoll::Poll(int slot, SlotState& state)
{
    ReadUserInfo(slot);
    if (!_rt.Hooks.ClientConVars.Available())
        return;

    const CvarRuleTable& rules = _detectors.InvalidCvars.Rules();
    const std::span<const CvarRule> queried = rules.Queried();
    if (queried.empty())
        return;

    // Asking for a convar already in flight re-points the outstanding request rather than sending a
    // second one, so the batch never has to check what is pending.
    for (size_t offset = 0; offset < CvarsPerPoll; ++offset)
    {
        _rt.Hooks.ClientConVars.Query(slot, queried[rules.PollCvarIndex(state.Cursor, offset)].name,
                                    [this](int replySlot, VoltMod::ClientConVarStatus status, std::string_view cvar,
                                           std::string_view value) { OnReply(replySlot, status, cvar, value); });
    }
    state.Cursor = rules.PollCvarIndex(state.Cursor, CvarsPerPoll);
}

void CvarPoll::ReadUserInfo(int slot)
{
    const bool enforce = _detectors.EnforceCheatCvars();
    for (const CvarRule& rule : _detectors.InvalidCvars.Rules().UserInfo())
    {
        const std::string_view value = _rt.World.NetChannels.GetUserInfoCvar(slot, rule.name);
        if (value.empty())
            continue;
        ReportVerdict(slot, _detectors.InvalidCvars.Observe(slot, rule.name, value, enforce));
    }
}

void CvarPoll::OnReply(int slot, VoltMod::ClientConVarStatus status, std::string_view name,
                                  std::string_view value)
{
    if (!_detectors.Enabled() || !_detectors.RuleEnabled(_detectors.InvalidCvars) ||
        !_detectors.IsEligible(slot))
        return;

    // Both strings borrow the decoded message. The rule copies whatever becomes evidence.
    const bool enforce = _detectors.EnforceCheatCvars();
    InvalidCvar& rules = _detectors.InvalidCvars;
    ReportVerdict(slot, status == VoltMod::ClientConVarStatus::Answered
                            ? rules.Observe(slot, name, value, enforce)
                            : rules.ObserveMissing(slot, name, VoltMod::Name(status), enforce));
}

}  // namespace Anticheat
