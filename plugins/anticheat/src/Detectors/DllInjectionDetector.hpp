#pragma once

// A stock client subscribes only to the events its HUD needs. Injected client code registers its
// own legacy listener and asks for events no HUD ever wants - a fingerprint the server can read
// without touching the client. No core: the check is one engine query plus a schedule.

#include "Core/DetectionData.hpp"
#include "Core/Samples.hpp"

#include <CS2Kit/Api.hpp>
#include <array>
#include <cstdint>

namespace Anticheat
{

class AntiCheatManager;

class DllInjectionDetector
{
public:
    DllInjectionDetector(AntiCheatManager& manager, CS2Kit::Runtime& runtime, DetectionDataManager& detections)
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
    CS2Kit::Runtime& _rt;
    DetectionDataManager& _detections;
    std::array<SlotState, MaxSlots> _slots{};
    CS2Kit::Subscription _scanTimer;
};

}  // namespace Anticheat
