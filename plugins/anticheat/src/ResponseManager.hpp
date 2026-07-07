#pragma once

#include "Detectors/Detection.hpp"

#include <CS2Kit/Api.hpp>
#include <map>
#include <string>

namespace Anticheat
{

/** Response ladder, config "anticheat.mode". */
enum class Mode
{
    Observe,  // log detections only
    Alert,    // observe + notify admins via admin-system (as_ac_alert)
    Ban       // alert + auto-ban at ban score (as_ac_ban)
};

/**
 * Consumes detector findings: accumulates per-slot per-detector decaying
 * scores, logs every finding, and escalates per the configured mode. Bans go
 * through admin-system's console bridge so persistence/kick/broadcast stay in
 * one place; a slot's ban is latched so it fires once.
 */
class ResponseManager
{
public:
    void Initialize();
    void Handle(int slot, const Detection& detection);

    /** Console dump of every tracked player's scores (anticheat_status). */
    void DumpStatus();

private:
    struct SlotResponse
    {
        std::map<std::string, CS2Kit::DecayingScore, std::less<>> Scores;
        double LastAlert = 0.0;
        bool BanIssued = false;
    };

    Mode CurrentMode() const;

    CS2Kit::PerSlot<SlotResponse> _slots;
};

}  // namespace Anticheat
