#pragma once

#include "Config.hpp"
#include "Core/DetectionData.hpp"

// Owns detector cores, engine adapters, global gates, and console commands.

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

#include <VoltMod/Api.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <tuple>
#include <vector>

namespace Anticheat
{

class AntiCheatManager
{
public:
    AntiCheatManager(VoltMod::Runtime& runtime, ConfigManager& config, DetectionDataManager& detections,
                     ResponseManager& response)
        : _rt(runtime),
          _config(config),
          _detections(detections),
          _response(response),
          _simulator(*this, runtime, config)
    {}

    void Initialize();

    /** Clear evidence on map changes and configuration reloads. */
    void ResetEvidence();
    void OnSlotChanged(int slot);

    void OnMapStart();

    void OnPlayerFullyConnected(VoltMod::Player* player);
    void OnPlayerSettingsChanged(VoltMod::Player* player);

    /**
     * Disabled globally or while `sv_cheats` is enabled outside test mode.
     */
    bool DetectionsEnabled() const;

    bool ModuleEnabled(DetectionKind kind) const;

    /** True when @p slot should be judged at all (a connected, non-bot human). */
    bool IsEligible(int slot);

    void Report(int slot, const std::optional<Finding>& finding);

    /** Cheat-protected client values only mean something once a disabled sv_cheats has reached them. */
    bool EnforceCheatCvars() const;

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
    void DumpCommand(int slot, const VoltMod::UserCmdView& cmd);
    void LogStatus() const;
    /** Update hostile-shot rules from `mp_teammates_are_enemies`. */
    void RefreshTeamRules();

    VoltMod::Runtime& _rt;
    ConfigManager& _config;
    DetectionDataManager& _detections;
    ResponseManager& _response;

    ShotCorrelatorCore _correlator;
    AimbotCore _aimbot{_correlator};
    AimlockCore _aimlock{_correlator};
    AntiAimCore _antiAim;
    SilentAimCore _silentAim;
    NamechangerCore _namechanger;
    InvalidCvarRules _invalidCvars;

    ShotCorrelator _feed{*this, _rt};
    NamechangerDetector _namechangerDetector{*this, _rt};
    DllInjectionDetector _dllInjection{*this, _rt, _detections};
    InvalidCvarDetector _invalidCvarPoller{*this, _rt};

    // Stamped when sv_cheats goes off, so replicated client values get time to catch up.
    double _cheatGraceUntil = 0.0;

    /** Cached because ConVarStorage keeps a stable value pointer and name lookup is not free. */
    mutable std::optional<VoltMod::ConVarStorage> _svCheats;
    VoltMod::ConVarStorage& CheatsConVar() const;

    VoltMod::PerSlot<int> _dumpTicks;  // remaining ticks to dump raw usercmds (anticheat_dumpcmd)
    CheatSimulator _simulator;

    /** Listener registrations, released together. Declared last: reverse member destruction
     *  stops the callbacks before the state they capture goes away. */
    std::vector<VoltMod::Subscription> _subs;
};

}  // namespace Anticheat
