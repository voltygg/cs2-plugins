#include "Detectors/NamechangerDetector.hpp"

#include "AntiCheatManager.hpp"

#include <VoltMod/Core/Slot.hpp>
#include <VoltMod/Core/Time.hpp>
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

void NamechangerDetector::Initialize()
{
    _pollTimer = _rt.Scheduler.Repeat(PollIntervalMs, [this] { OnPoll(); });
}

// Baselining is bookkeeping, not detection, so it runs even while detections are gated off:
// a change measured against a stale identity would be a false positive later.
void NamechangerDetector::OnFullyConnected(VoltMod::Player& player)
{
    if (!_manager.ModuleEnabled(DetectionKind::Namechanger) || !_manager.IsEligible(player.Slot()))
        return;
    _manager.Namechanger().OnBaseline(player.Slot(), player.Name(), ClanOf(player.Ctrl()));
}

bool NamechangerDetector::Enabled() const
{
    return _manager.DetectionsEnabled() && _manager.ModuleEnabled(DetectionKind::Namechanger);
}

void NamechangerDetector::OnSettingsChanged(VoltMod::Player& player)
{
    if (!Enabled())
        return;
    CheckIdentity(player.Slot(), Time::MonotonicSeconds());
}

void NamechangerDetector::OnPoll()
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

void NamechangerDetector::CheckIdentity(int slot, double nowSec)
{
    if (!_manager.IsEligible(slot))
        return;

    // The controller carries the name and tag the scoreboard shows right now; NamechangerCore
    // compares them against the baseline it holds.
    const VoltMod::Controller controller = _rt.Entities.Controller(slot);
    if (!controller)
        return;
    _manager.Report(slot, _manager.Namechanger().OnIdentity(slot, controller.Name(), ClanOf(controller), nowSec));
}

}  // namespace Anticheat
