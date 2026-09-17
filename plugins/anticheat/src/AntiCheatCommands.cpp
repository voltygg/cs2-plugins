#include "AntiCheatManager.hpp"

#include "App.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Slot.hpp>
#include <VoltMod/Core/Time.hpp>
#include <format>
#include <string>

using VoltMod::Caller;
using VoltMod::IsValidSlot;
using VoltMod::Reply;
using VoltMod::Result;
using VoltMod::Time;

namespace Args = VoltMod::Args;
namespace Log = VoltMod::Log;

namespace Anticheat
{

static constexpr int DefaultDumpTicks = 64;
static constexpr int MaxDumpTicks = 10000;

void AntiCheatManager::RegisterCommands()
{
    auto& commands = _rt.Commands;

    commands.Add("anticheat_reload")
        .Describe("Re-read settings.jsonc and detections.jsonc, and drop all accumulated evidence.")
        .ConsoleOnly()
        .Run([this](Caller) -> Result<Reply> {
            // The reason names the offending key and its position, which is what a mistyped setting needs.
            if (auto loaded = _config.Load(VoltMod::AddonFile(AddonName, "configs/settings.jsonc")); !loaded)
                return Reply{std::format("Settings not reloaded: {}", loaded.error().Detail)};
            // Keep valid rules active if the edited file cannot be parsed.
            if (auto loaded = _detections.Load(DetectionDataPath); !loaded)
                Log::Warn("{} could not be re-read ({}); keeping the tables already loaded.", DetectionDataPath,
                          loaded.error().Detail);
            else
                LoadDetectionData();
            _detectors.RefreshTeamRules();
            ResetEvidence();
            return Reply{std::format("Settings reloaded (mode={}); evidence cleared.", _config.Get().anticheat.mode)};
        });

    commands.Add("anticheat_status")
        .Describe("Print the module state and per-player detection evidence.")
        .ConsoleOnly()
        .Run([this](Caller caller) -> Result<Reply> {
            // One line per call, so a remote console that keeps the first line still sees them all.
            for (const std::string& line : StatusReport())
                caller.SayRaw(line);
            return Reply::Silent();
        });

    commands.Add("anticheat_dumpcmd")
        .Describe("Log raw usercmds for a slot.")
        .ConsoleOnly()
        .Run([this](Caller, Args::Int slot, Args::Opt<Args::Int> requested) -> Result<Reply> {
            if (!IsValidSlot(slot.Value))
                return Reply{std::format("anticheat_dumpcmd: {} is not a valid slot.", slot.Value)};

            const int ticks = requested.Value ? requested.Value->Value : DefaultDumpTicks;
            if (ticks < 1 || ticks > MaxDumpTicks)
                return Reply{std::format("anticheat_dumpcmd: ticks must be 1-{}.", MaxDumpTicks)};

            _dumpTicks[slot.Value] = ticks;
            return Reply{std::format("Dumping {} usercmds for slot {}.", ticks, slot.Value)};
        });
}

std::vector<std::string> AntiCheatManager::StatusReport() const
{
    std::vector<std::string> report{std::format("[AC] {}", StatusSnapshot())};

    const double now = Time::MonotonicSeconds();
    const Detectors& detectors = _detectors;
    bool any = false;
    for (const VoltMod::Player* player : _rt.Players.All())
    {
        const int slot = player ? player->Slot() : -1;
        if (!InSlotRange(slot) || (player->IsBot() && !detectors.IncludesBots()))
            continue;
        any = true;

        std::string latched;
        const std::span<const CvarRule> rules = detectors.InvalidCvars.Rules().All();
        for (size_t index = 0; index < rules.size(); ++index)
        {
            if (!detectors.InvalidCvars.IsLatchedAt(slot, index))
                continue;
            if (!latched.empty())
                latched += ",";
            latched += rules[index].name;
        }

        report.push_back(std::format(
            "[AC] s{} {} ({}) punished={} aimbot={} aimlock={}{} antiaim={:.1f} silentaim={} trigger={} recoil={}{} "
            "mouse={}{} wallhack={}{} names={} cvars=[{}] pending={} poll={:.1f}s shots={} cmds={} gen={}",
            slot, player->Name(), player->SteamId(), PunishmentName(_response.Issued(slot)),
            detectors.Aimbot.IncidentCount(slot), detectors.Aimlock.IncidentCount(slot),
            detectors.Aimlock.IsTracking(slot) ? "/tracking" : "", detectors.AntiAim.Score(slot),
            detectors.SilentAim.Score(slot, now), detectors.Triggerbot.Score(slot, now), detectors.Recoil.Score(slot, now),
            detectors.Recoil.InSpray(slot) ? "/spraying" : "", detectors.Mouse.Score(slot, now),
            detectors.Mouse.Calibrated(slot) ? "" : "/uncalibrated", detectors.Wallhack.Score(slot, now),
            detectors.Wallhack.IsTracking(slot) ? "/tracking" : "", detectors.Namechanger.ChangeCount(slot),
            latched.empty() ? "-" : latched, _rt.Hooks.ClientConVars.PendingCount(slot),
            _cvarPoll.PollsIn(slot, now), detectors.History.Shots(slot).size(),
            detectors.History.CommandCount(slot), detectors.History.Generation(slot)));
    }
    if (!any)
        report.push_back(detectors.IncludesBots() ? "[AC] no players connected." : "[AC] no human players connected.");
    return report;
}

}  // namespace Anticheat
