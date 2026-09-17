#pragma once

#include "Config.hpp"
#include "Engine/CheatSimulator.hpp"
#include "Engine/CommandDump.hpp"
#include "Engine/CvarPoll.hpp"
#include "Engine/DetectionDataManager.hpp"
#include "Engine/DetectionFeed.hpp"
#include "Engine/Detectors.hpp"
#include "Engine/DllInjectionScan.hpp"
#include "Engine/NamechangerPoll.hpp"
#include "Engine/SuspicionSnapshot.hpp"
#include "Response/DiscordReporter.hpp"
#include "Response/ResponseManager.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Subscriptions.hpp>
#include <tuple>

namespace Anticheat
{

/**
 * Everything this plugin owns for one Load/Unload cycle, and the wiring between it. The plugin
 * creates it in OnLoad and drops it in OnUnload, so no state survives a `meta reload`.
 *
 * Members are declared in dependency order and destroyed in reverse; each is handed the
 * collaborators it needs.
 */
struct App
{
    explicit App(VoltMod::Runtime& runtime) : Runtime(runtime) {}

    /** Load config and detection data, then arm the detection rules. */
    bool Start();

    /** Push configs/detections.jsonc into the two table-driven rules. */
    void LoadDetectionData();
    /** Drop in-flight detector state whose ticks and positions no longer mean anything. */
    void ResetInFlight();
    /** Also drop every player's accumulated suspicion, for the operator reload that asks for it. */
    void ForgetEvidence();
    void OnMapStart();

    VoltMod::Runtime& Runtime;
    ConfigManager Config;
    DetectionDataManager RuleTables;
    DiscordReporter Reporter{Runtime, Config};
    ResponseManager Response{Runtime, Config, Reporter};

    Detectors Detection{Runtime, Config, Response};
    DetectionFeed Feed{Detection, Runtime};
    NamechangerPoll Names{Detection, Runtime};
    DllInjectionScan DllScan{Detection, Runtime, RuleTables};
    CvarPoll Cvars{Detection, Runtime};
    CheatSimulator Simulator{Detection, Runtime, Config};
    CommandDump Dump{Runtime};
    SuspicionSnapshot Carried{Detection, Runtime};

private:
    void OnSlotChanged(int slot);
    void OnPlayerFullyConnected(VoltMod::Player& player);

    /** Everything a reset or a slot change has to clear; a new adapter is wired in here only. */
    auto Modules() { return std::tie(Detection, DllScan, Cvars); }

    /** Listener registrations, released together. Declared last: reverse member destruction
     *  stops the callbacks before the state they capture goes away. */
    VoltMod::Subscriptions _subs;
};

}  // namespace Anticheat
