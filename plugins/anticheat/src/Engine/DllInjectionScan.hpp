#pragma once

#include "Engine/Detectors.hpp"
#include "Engine/DetectionDataManager.hpp"
#include "Detect/Samples.hpp"

#include <VoltMod/Api.hpp>
#include <array>
#include <cstdint>

namespace Anticheat
{

class DllInjectionScan
{
public:
    DllInjectionScan(Detectors& detectors, VoltMod::Runtime& runtime, DetectionDataManager& detections)
        : _detectors(detectors), _rt(runtime), _detections(detections)
    {}

    /** Start the repeating scan timer. Idempotent. */
    void Initialize();

    void OnSlotChanged(int slot);
    void Reset();

private:
    struct SlotState
    {
        double NextScan = 0.0;  // 0 = not scheduled
        bool Retried = false;   // the one grace scan for a client whose listener was not up yet
    };

    void Scan(int slot, SlotState& state, double nowSec);

    Detectors& _detectors;
    VoltMod::Runtime& _rt;
    DetectionDataManager& _detections;
    std::array<SlotState, MaxSlots> _slots{};
    VoltMod::Subscription _scanTimer;
};

}  // namespace Anticheat
