#include "DllInjectionDetector.hpp"

#include "AntiCheatManager.hpp"
#include "App.hpp"

#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace Anticheat
{

namespace
{
/** Pump cadence only; the per-slot deadlines do the real timing. */
constexpr int64_t ScanIntervalMs = 1000;
/** First scan this long after full connect; retried once when no listener exists yet. */
constexpr double DllInitialScanDelaySec = 10.0;
/** Rescan cadence once the first scan succeeded. */
constexpr double DllScanIntervalSec = 120.0;
/** Evidence string cap before the matched names are elided. */
constexpr size_t DllEvidenceCharBudget = 700;
}  // namespace

void DllInjectionDetector::Initialize()
{
    if (_scanTimer)
        return;

    _scanTimer = _rt.Scheduler.Repeat(ScanIntervalMs, [this] {
        if (!_manager.DetectionsEnabled() || !_manager.ModuleEnabled(DetectionKind::DllInjection))
            return;

        const double now = TimeUtils::MonotonicSeconds();
        for (int slot = 0; slot < MaxSlots; ++slot)
        {
            SlotState& state = _slots[slot];
            if (!_manager.IsEligible(slot))
                continue;
            // A map change clears every schedule, and players who ride it out never connect again.
            if (state.NextScan == 0.0)
            {
                state.NextScan = now + DllInitialScanDelaySec;
                continue;
            }
            if (now < state.NextScan)
                continue;
            Scan(slot, state, now);
        }
    });
}

void DllInjectionDetector::OnFullyConnected(int slot)
{
    if (!InSlotRange(slot))
        return;
    // The client's listener does not exist the instant it joins, so the first scan waits for it.
    _slots[slot] = {.NextScan = TimeUtils::MonotonicSeconds() + DllInitialScanDelaySec};
}

void DllInjectionDetector::OnSlotChanged(int slot)
{
    if (InSlotRange(slot))
        _slots[slot] = {};
}

void DllInjectionDetector::Reset()
{
    _slots = {};
}

void DllInjectionDetector::Scan(int slot, SlotState& state, double nowSec)
{
    if (!_rt.Events.GetClientLegacyListener(slot))
    {
        // One grace period for a client still settling in. After that a missing listener is simply
        // nothing to scan, and the slot falls back to the normal cadence.
        state.NextScan = nowSec + (state.Retried ? DllScanIntervalSec : DllInitialScanDelaySec);
        state.Retried = true;
        return;
    }

    state.NextScan = nowSec + DllScanIntervalSec;
    state.Retried = true;

    // Read through at scan time rather than holding a copy: one owner for the table, and a reload
    // cannot leave this detector checking a stale list.
    std::vector<std::string_view> matches;
    for (const std::string& name : _detections.Get().dllEventBlacklist)
        if (_rt.Events.ClientListensTo(slot, name.c_str()))
            matches.push_back(name);
    if (matches.empty())
        return;

    std::string evidence;
    for (std::string_view name : matches)
    {
        if (evidence.size() > DllEvidenceCharBudget)
        {
            evidence += ", ...";
            break;
        }
        if (!evidence.empty())
            evidence += ", ";
        evidence += name;
    }

    _manager.Report(slot, Finding{.Kind = DetectionKind::DllInjection,
                                  .Evidence = std::format("{} blacklisted client event subscription{} found: {}.",
                                                          matches.size(), matches.size() == 1 ? "" : "s", evidence)});
}

}  // namespace Anticheat
