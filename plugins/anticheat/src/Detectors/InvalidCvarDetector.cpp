#include "InvalidCvarDetector.hpp"

#include "AntiCheatManager.hpp"

#include <algorithm>
#include <chrono>
#include <string>

using CS2Kit::Core::Engine;

namespace Anticheat
{

namespace
{
/** How often the pump looks for slots whose poll is due; the per-slot deadlines do the real timing. */
constexpr int64_t PumpIntervalMs = 1000;

/** A seed in [1, m-1], the only range std::minstd_rand accepts. */
std::minstd_rand::result_type Seed()
{
    const auto ticks = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    return static_cast<std::minstd_rand::result_type>(ticks % 2147483646U) + 1;
}
}  // namespace

void InvalidCvarDetector::Initialize()
{
    if (_pump != 0)
        return;

    _random.seed(Seed());
    _pump = Engine().Scheduler.Repeat(PumpIntervalMs, [this] {
        if (!_manager.DetectionsEnabled() || !AntiCheatManager::ModuleEnabled(DetectionKind::InvalidCvar))
            return;
        const double now = NowSeconds();
        for (int slot = 0; slot < MaxSlots; ++slot)
        {
            SlotState& state = _slots[slot];
            if (!AntiCheatManager::IsEligible(slot))
                continue;
            // A map change clears every schedule, and players who ride it out never connect again,
            // so an eligible slot without one arms itself here rather than waiting forever.
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
        _slots[slot] = {.NextPoll = NowSeconds() + NextDelaySec()};
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
    if (!Engine().ClientCvars.Available())
        return;

    // Asking for a convar that is already in flight re-points the outstanding request instead of
    // sending a second one, so the batch never has to check what is pending.
    for (size_t offset = 0; offset < CvarsPerPoll; ++offset)
    {
        const std::string name(QueriedCvars[PollCvarIndex(state.Cursor, offset)]);
        Engine().ClientCvars.Query(slot, name,
                                   [this](int replySlot, CS2Kit::ClientCvarStatus status, std::string_view cvar,
                                          std::string_view value) { OnReply(replySlot, status, cvar, value); });
    }
    state.Cursor = PollCvarIndex(state.Cursor, CvarsPerPoll);
}

void InvalidCvarDetector::ReadUserInfo(int slot)
{
    const bool enforce = _manager.EnforceCheatCvars();
    for (std::string_view name : UserInfoCvars)
    {
        const char* value = Engine().NetChannels.GetUserInfoCvar(slot, name.data());
        if (!value || *value == '\0')
            continue;
        _manager.Report(slot, _manager.InvalidCvars().Observe(slot, name, value, enforce));
    }
}

void InvalidCvarDetector::OnReply(int slot, CS2Kit::ClientCvarStatus status, std::string_view name,
                                  std::string_view value)
{
    if (!_manager.DetectionsEnabled() || !AntiCheatManager::ModuleEnabled(DetectionKind::InvalidCvar) ||
        !AntiCheatManager::IsEligible(slot))
        return;

    // Both strings borrow the decoded message; the rules core copies whatever ends up as evidence.
    const bool enforce = _manager.EnforceCheatCvars();
    InvalidCvarRules& rules = _manager.InvalidCvars();
    _manager.Report(slot, status == CS2Kit::ClientCvarStatus::ValueIntact
                              ? rules.Observe(slot, name, value, enforce)
                              : rules.ObserveMissing(slot, name, CS2Kit::Sdk::ToString(status), enforce));
}

}  // namespace Anticheat
