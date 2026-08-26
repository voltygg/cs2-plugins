#include "NamechangerDetector.hpp"

#include "AntiCheatManager.hpp"

#include <VoltMod/Core/Slot.hpp>
#include <VoltMod/Runtime.hpp>

namespace Anticheat
{

// Baselining is bookkeeping, not judgement, so it runs even while detections are gated off:
// a change measured against a stale name would be a false positive later.
void NamechangerDetector::OnFullyConnected(VoltMod::Player& player)
{
    if (!_manager.ModuleEnabled(DetectionKind::Namechanger) || !_manager.IsEligible(player.Slot()))
        return;
    _manager.Namechanger().OnBaseline(player.Slot(), player.Name());
}

void NamechangerDetector::OnSettingsChanged(VoltMod::Player& player)
{
    if (!_manager.DetectionsEnabled() || !_manager.ModuleEnabled(DetectionKind::Namechanger) ||
        !_manager.IsEligible(player.Slot()))
        return;

    // Player::Name reads the controller, so this is the name the scoreboard shows right now;
    // NamechangerCore compares it against the baseline it holds.
    const int slot = player.Slot();
    _manager.Report(slot, _manager.Namechanger().OnNameChanged(slot, player.Name(), Time::MonotonicSeconds()));
}

}  // namespace Anticheat
