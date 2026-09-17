#pragma once

#include "Detect/Samples.hpp"
#include "Engine/DetectionDataManager.hpp"
#include "Engine/Detectors.hpp"
#include "Engine/SlotSchedule.hpp"

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

    void ClearSlot(int slot);
    void Reset();

private:
    void Scan(int slot, double nowSec);

    Detectors& _detectors;
    VoltMod::Runtime& _rt;
    DetectionDataManager& _detections;
    SlotSchedule _schedule;
    /** The one grace scan each slot gets while its client's listener is not up yet. */
    std::array<bool, MaxSlots> _retried{};
    VoltMod::Subscription _scanTimer;
};

}  // namespace Anticheat
