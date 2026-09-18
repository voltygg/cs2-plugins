#include "Engine/NamechangerPoll.hpp"

#include <VoltMod/Core/Slots/Slot.hpp>
#include <VoltMod/Core/Time/Time.hpp>
#include <string_view>

namespace Anticheat
{

using VoltMod::Time;

/** A tag animation faster than this still lands five changes a minute. */
static constexpr int64_t PollIntervalMs = 125;

static std::string_view ClanOf(const VoltMod::Controller& controller)
{
    const char* clan = controller ? controller.Clan() : nullptr;
    return clan ? std::string_view(clan) : std::string_view{};
}

void NamechangerPoll::Initialize()
{
    _pollTimer = _rt.Scheduler.Repeat(PollIntervalMs, [this] { OnPoll(); });
}

// Baselining is bookkeeping, not detection, so it runs even while detections are gated off:
// a change measured against a stale identity would be a false positive later.
void NamechangerPoll::OnFullyConnected(VoltMod::Player& player)
{
    if (!_detectors.RuleEnabled(_detectors.Namechanger) || !_detectors.IsEligible(player.Slot()))
        return;
    _detectors.Namechanger.OnBaseline(player.Slot(), player.Name(), ClanOf(player.Ctrl()));
}

bool NamechangerPoll::Enabled() const
{
    return _detectors.Enabled() && _detectors.RuleEnabled(_detectors.Namechanger);
}

void NamechangerPoll::OnSettingsChanged(VoltMod::Player& player)
{
    if (!Enabled())
        return;
    CheckIdentity(player.Slot(), Time::MonotonicSeconds());
}

void NamechangerPoll::OnPoll()
{
    if (!Enabled())
        return;

    const double now = Time::MonotonicSeconds();
    for (const VoltMod::Player* player : _rt.Players.All())
    {
        if (player)
            CheckIdentity(player->Slot(), now);
    }
}

void NamechangerPoll::CheckIdentity(int slot, double nowSec)
{
    if (!_detectors.IsEligible(slot))
        return;

    // The controller carries the name and tag the scoreboard shows right now; Namechanger
    // compares them against the baseline it holds.
    const VoltMod::Controller controller = _rt.Entities.Controller(slot);
    if (!controller)
        return;
    _detectors.Namechanger.OnIdentity(slot, controller.Name(), ClanOf(controller), nowSec);
}

}  // namespace Anticheat
