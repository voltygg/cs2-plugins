#pragma once

// Detect injected listeners that subscribe to events unused by the stock HUD.

#include "Core/DetectionData.hpp"
#include "Core/Samples.hpp"

#include <VoltMod/Api.hpp>
#include <array>
#include <cstdint>

namespace Anticheat
{

class AntiCheatManager;

class DllInjectionDetector
{
public:
    DllInjectionDetector(AntiCheatManager& manager, VoltMod::Runtime& runtime, DetectionDataManager& detections)
        : _manager(manager), _rt(runtime), _detections(detections)
    {}

    /** Start the scan pump. Idempotent. */
    void Initialize();

    /** A player is in the server: arm their first scan. */
    void OnFullyConnected(int slot);

    void OnSlotChanged(int slot);
    void Reset();

private:
    struct SlotState
    {
        double NextScan = 0.0;  // 0 = not armed
        bool Retried = false;   // the one grace scan for a client whose listener was not up yet
    };

    void Scan(int slot, SlotState& state, double nowSec);

    AntiCheatManager& _manager;
    VoltMod::Runtime& _rt;
    DetectionDataManager& _detections;
    std::array<SlotState, MaxSlots> _slots{};
    VoltMod::Subscription _scanTimer;
};

}  // namespace Anticheat
