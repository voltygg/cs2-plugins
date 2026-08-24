#include "NamechangerDetector.hpp"

#include "AntiCheatManager.hpp"

#include <VoltMod/Core/Slot.hpp>

namespace Anticheat
{

std::string NamechangerDetector::CurrentName(VoltMod::Players::Player* player)
{
    std::string name = VoltMod::PlayerController(player->GetSlot()).GetPlayerName();
    return name.empty() ? player->GetName() : name;
}

// Baselining is bookkeeping, not judgement, so it runs even while detections are gated off:
// a change measured against a stale name would be a false positive later.
void NamechangerDetector::OnFullyConnected(VoltMod::Players::Player* player)
{
    if (!player || !_manager.ModuleEnabled(DetectionKind::Namechanger) || !_manager.IsEligible(player->GetSlot()))
        return;
    _manager.Namechanger().OnBaseline(player->GetSlot(), CurrentName(player));
}

void NamechangerDetector::OnSettingsChanged(VoltMod::Players::Player* player)
{
    if (!player || !_manager.DetectionsEnabled() || !_manager.ModuleEnabled(DetectionKind::Namechanger) ||
        !_manager.IsEligible(player->GetSlot()))
        return;

    const int slot = player->GetSlot();
    _manager.Report(slot,
                    _manager.Namechanger().OnNameChanged(slot, CurrentName(player), TimeUtils::MonotonicSeconds()));
}

}  // namespace Anticheat
