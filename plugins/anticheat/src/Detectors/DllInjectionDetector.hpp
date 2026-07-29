#pragma once

// A stock client subscribes only to the events its HUD needs. Injected client code registers its
// own legacy listener and asks for events no HUD ever wants - a fingerprint the server can read
// without touching the client. No core: the check is one engine query plus a schedule.

#include "Core/Samples.hpp"

#include <CS2Kit/Api.hpp>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace Anticheat
{

class AntiCheatManager;

class DllInjectionDetector
{
public:
    explicit DllInjectionDetector(AntiCheatManager& manager) : _manager(manager) {}

    /** Start the scan pump. Idempotent. */
    void Initialize();

    /** The event names to look for, from configs/detections.jsonc. Empty scans nothing. */
    void SetBlacklist(std::vector<std::string> events) { _blacklist = std::move(events); }

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
    std::vector<std::string> _blacklist;
    std::array<SlotState, MaxSlots> _slots{};
    uint64_t _pump = 0;
};

}  // namespace Anticheat
