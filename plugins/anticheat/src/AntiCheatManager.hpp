#pragma once

// Owns the detection cores, the adapters that feed them, and the console surface. The sv_cheats
// gate, the eligibility rule and evidence reset are all global, so they live here.

#include "Correlation/ShotCorrelator.hpp"
#include "Correlation/ShotCorrelatorCore.hpp"
#include "Detectors/AimbotCore.hpp"
#include "Detectors/AimlockCore.hpp"
#include "Detectors/AntiAimCore.hpp"
#include "Detectors/DllInjectionDetector.hpp"
#include "Detectors/InvalidCvarDetector.hpp"
#include "Detectors/NamechangerCore.hpp"
#include "Detectors/NamechangerDetector.hpp"
#include "Detectors/SilentAimCore.hpp"
#include "Response/ResponseManager.hpp"
#include "Simulator/CheatSimulator.hpp"

#include <CS2Kit/Api.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <tuple>

namespace Anticheat
{

class AntiCheatManager
{
public:
    explicit AntiCheatManager(ResponseManager& response) : _response(response) {}

    void Initialize();

    /** Map change, config reload or disconnect: nothing accumulated may survive. */
    void ResetEvidence();
    void OnSlotChanged(int slot);

    /** Pawns, positions and ticks all restart, so no evidence carries over. */
    void OnMapStart();

    void OnPlayerFullyConnected(CS2Kit::Players::Player* player);
    void OnPlayerSettingsChanged(CS2Kit::Players::Player* player);

    /**
     * Master gate. Off while the plugin is disabled, and while sv_cheats is on unless the operator
     * opted into testing - cheat-protected client state legitimately changes under sv_cheats.
     */
    bool DetectionsEnabled() const;

    static bool ModuleEnabled(DetectionKind kind);

    /** True when @p slot should be judged at all (a connected, non-bot human). */
    static bool IsEligible(int slot);

    void Report(int slot, const std::optional<Finding>& finding);

    /** Cheat-protected client values only mean something once a disabled sv_cheats has reached them. */
    bool EnforceCheatCvars() const;

    /** The `anticheat` section of Engine().Status. */
    nlohmann::json StatusSnapshot() const;

    ShotCorrelatorCore& Correlator() { return _correlator; }
    AimbotCore& Aimbot() { return _aimbot; }
    AimlockCore& Aimlock() { return _aimlock; }
    AntiAimCore& AntiAim() { return _antiAim; }
    SilentAimCore& SilentAim() { return _silentAim; }
    NamechangerCore& Namechanger() { return _namechanger; }
    InvalidCvarRules& InvalidCvars() { return _invalidCvars; }

private:
    /** Everything a reset or a slot change has to clear. Both fan out over this, so a new core is
     *  wired into them here and nowhere else. */
    auto ResettableModules()
    {
        return std::tie(_correlator, _aimbot, _aimlock, _antiAim, _silentAim, _namechanger, _invalidCvars,
                        _dllInjection, _invalidCvarPoller, _response);
    }

    void RegisterCommands();
    /** Push configs/detections.jsonc into the two table-driven modules. */
    void LoadDetectionData();
    void DumpCommand(int slot, const CS2Kit::UserCmdView& cmd);
    /** anticheat_status: the snapshot, then one line per human player. */
    void LogStatus() const;
    /** Pull mp_teammates_are_enemies into the correlator: it decides which shots are hostile. */
    void RefreshTeamRules();

    ResponseManager& _response;

    ShotCorrelatorCore _correlator;
    AimbotCore _aimbot{_correlator};
    AimlockCore _aimlock{_correlator};
    AntiAimCore _antiAim;
    SilentAimCore _silentAim;
    NamechangerCore _namechanger;
    InvalidCvarRules _invalidCvars;

    ShotCorrelator _feed{*this};
    NamechangerDetector _namechangerDetector{*this};
    DllInjectionDetector _dllInjection{*this};
    InvalidCvarDetector _invalidCvarPoller{*this};

    // Stamped when sv_cheats goes off, so replicated client values get time to catch up.
    double _cheatGraceUntil = 0.0;

    /** Resolved once instead of per usercmd: RawConVar holds the convar's value pointer, which
     *  stays valid, and the by-name lookup behind Raw() is not free. */
    mutable std::optional<CS2Kit::RawConVar> _svCheats;
    CS2Kit::RawConVar& CheatsConVar() const;

    CS2Kit::PerSlot<int> _dumpTicks;  // remaining ticks to dump raw usercmds (anticheat_dumpcmd)
    std::optional<CS2Kit::ServerCommand> _cmdReload;
    std::optional<CS2Kit::ServerCommand> _cmdStatus;
    std::optional<CS2Kit::ServerCommand> _cmdDump;
    CheatSimulator _simulator;
};

}  // namespace Anticheat
