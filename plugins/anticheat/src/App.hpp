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
#include <VoltMod/Core/Signals/Subscriptions.hpp>

namespace Anticheat
{

/**
 * Everything this plugin owns for one load cycle, so no state survives a `volt reload`. Members
 * are declared in dependency order and destroyed in reverse.
 */
struct App final : VoltMod::Plugin
{
    explicit App(VoltMod::Runtime& runtime) : Plugin(runtime) {}

    /** Load config and detection data, then arm the detection rules. */
    bool Load() override;

    /** Push configs/detections.jsonc into the two table-driven rules. */
    void LoadDetectionData();

    /** A new map invalidates every tick and position sampled on the old one. Scores survive it. */
    void OnServerStartup(std::string_view mapName) override;

    /** The seat changed hands: everything keyed to it goes, the previous player's score included. */
    void ClearSlot(int slot);

    /** The operator reset: tracking, every score, and the scores held between sessions. */
    void ClearAll();

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
    void OnPlayerFullyConnected(VoltMod::Player& player);

    /** Part-built episodes in the detectors, and the poll schedules that feed them. */
    void ClearTracking();

    /** Listener registrations, released together. Declared last: reverse member destruction
     *  stops the callbacks before the state they capture goes away. */
    VoltMod::Subscriptions _subs;
};

}  // namespace Anticheat
