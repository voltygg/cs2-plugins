#include "InvalidCvarDetector.hpp"

#include "AntiCheatManager.hpp"

#include <algorithm>
#include <chrono>
#include <string>

namespace Anticheat
{

using VoltMod::Time;

/** Pump cadence only; the per-slot deadlines do the real timing. */
static constexpr int64_t PollIntervalMs = 1000;

/** A seed in [1, m-1], the only range std::minstd_rand accepts. */
static std::minstd_rand::result_type Seed()
{
    const auto ticks = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    return static_cast<std::minstd_rand::result_type>(ticks % 2147483646U) + 1;
}

void InvalidCvarDetector::Initialize()
{
    if (_pollTimer)
        return;

    _random.seed(Seed());
    _pollTimer = _rt.Scheduler.Repeat(PollIntervalMs, [this] {
        if (!_manager.DetectionsEnabled() || !_manager.ModuleEnabled(DetectionKind::InvalidCvar))
            return;
        const double now = Time::MonotonicSeconds();
        for (int slot = 0; slot < MaxSlots; ++slot)
        {
            SlotState& state = _slots[slot];
            if (!_manager.IsEligible(slot))
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

void InvalidCvarDetector::OnFullyConnected(int slot)
{
    if (InSlotRange(slot))
        _slots[slot] = {.NextPoll = Time::MonotonicSeconds() + NextDelaySec()};
}

void InvalidCvarDetector::OnSlotChanged(int slot)
{
    if (InSlotRange(slot))
        _slots[slot] = {};
}

void InvalidCvarDetector::Reset()
{
    _slots = {};
}

double InvalidCvarDetector::PollsIn(int slot, double nowSec) const
{
    if (!InSlotRange(slot) || _slots[slot].NextPoll == 0.0)
        return 0.0;
    return std::max(0.0, _slots[slot].NextPoll - nowSec);
}

double InvalidCvarDetector::NextDelaySec()
{
    return PollDelaySec(std::generate_canonical<double, 24>(_random));
}

void InvalidCvarDetector::Poll(int slot, SlotState& state)
{
    ReadUserInfo(slot);
    if (!_rt.Capabilities.Has(VoltMod::Capability::ClientCvars))
        return;

    const CvarRuleTable& rules = _manager.InvalidCvars().Rules();
    const std::span<const CvarRule> queried = rules.Queried();
    if (queried.empty())
        return;

    // Asking for a convar already in flight re-points the outstanding request rather than sending a
    // second one, so the batch never has to check what is pending.
    for (size_t offset = 0; offset < CvarsPerPoll; ++offset)
    {
        _rt.Hooks.ClientCvars.Query(slot, queried[rules.PollCvarIndex(state.Cursor, offset)].name,
                                    [this](int replySlot, VoltMod::ClientCvarStatus status, std::string_view cvar,
                                           std::string_view value) { OnReply(replySlot, status, cvar, value); });
    }
    state.Cursor = rules.PollCvarIndex(state.Cursor, CvarsPerPoll);
}

void InvalidCvarDetector::ReadUserInfo(int slot)
{
    const bool enforce = _manager.EnforceCheatCvars();
    for (const CvarRule& rule : _manager.InvalidCvars().Rules().UserInfo())
    {
        const std::string_view value = _rt.World.NetChannels.GetUserInfoCvar(slot, rule.name);
        if (value.empty())
            continue;
        _manager.Report(slot, _manager.InvalidCvars().Observe(slot, rule.name, value, enforce));
    }
}

void InvalidCvarDetector::OnReply(int slot, VoltMod::ClientCvarStatus status, std::string_view name,
                                  std::string_view value)
{
    if (!_manager.DetectionsEnabled() || !_manager.ModuleEnabled(DetectionKind::InvalidCvar) ||
        !_manager.IsEligible(slot))
        return;

    // Both strings borrow the decoded message. The rules core copies whatever becomes evidence.
    const bool enforce = _manager.EnforceCheatCvars();
    InvalidCvarRules& rules = _manager.InvalidCvars();
    _manager.Report(slot, status == VoltMod::ClientCvarStatus::ValueIntact
                              ? rules.Observe(slot, name, value, enforce)
                              : rules.ObserveMissing(slot, name, VoltMod::Name(status), enforce));
}

}  // namespace Anticheat
