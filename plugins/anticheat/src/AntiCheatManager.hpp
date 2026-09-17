#pragma once

#include "Engine/DllInjectionScan.hpp"
#include "Engine/CvarPoll.hpp"
#include "Engine/NamechangerPoll.hpp"
#include "Config.hpp"
#include "Engine/DetectionDataManager.hpp"
#include "Engine/DetectionFeed.hpp"
#include "Engine/Detectors.hpp"
#include "Response/ResponseManager.hpp"
#include "Engine/CheatSimulator.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Subscriptions.hpp>
#include <string>
#include <tuple>
#include <vector>

namespace Anticheat
{

class AntiCheatManager
{
public:
    AntiCheatManager(VoltMod::Runtime& runtime, ConfigManager& config, DetectionDataManager& detections,
                     ResponseManager& response)
        : _rt(runtime), _config(config), _detections(detections), _response(response)
    {}

    void Initialize();

    /** Clear evidence on map changes and configuration reloads. */
    void ResetEvidence();
    void OnMapStart();

    /** This plugin's `status` section as compact JSON text. Text, not a document, so this
     *  header stays clear of the JSON library. */
    std::string StatusSnapshot() const;

private:
    void OnSlotChanged(int slot);
    void OnPlayerFullyConnected(VoltMod::Player& player);

    /** Defined in AntiCheatCommands.cpp with the status report it prints. */
    void RegisterCommands();
    /** Push configs/detections.jsonc into the two table-driven modules. */
    void LoadDetectionData();
    void DumpCommand(int slot, const VoltMod::PlayerInput& cmd);
    /** The module state and per-player evidence, one line each. */
    std::vector<std::string> StatusReport() const;

    VoltMod::Runtime& _rt;
    ConfigManager& _config;
    DetectionDataManager& _detections;
    ResponseManager& _response;

    Detectors _detectors{_rt, _config, _response};

    DetectionFeed _feed{_detectors, _rt};
    NamechangerPoll _namechangerPoll{_detectors, _rt};
    DllInjectionScan _dllInjection{_detectors, _rt, _detections};
    CvarPoll _cvarPoll{_detectors, _rt};

    VoltMod::PerSlot<int> _dumpTicks;  // remaining ticks to dump raw usercmds (anticheat_dumpcmd)
    CheatSimulator _simulator{_detectors, _rt, _config};

    /** Everything a reset or a slot change has to clear; a new module is wired in here only. */
    auto Modules()
    {
        return std::tie(_detectors, _dllInjection, _cvarPoll, _response);
    }

    /** Listener registrations, released together. Declared last: reverse member destruction
     *  stops the callbacks before the state they capture goes away. */
    VoltMod::Subscriptions _subs;
};

}  // namespace Anticheat
