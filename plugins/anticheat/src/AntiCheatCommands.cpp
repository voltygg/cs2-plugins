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
            // The reason names the offending key and its position, which is what an operator
            // who just mistyped a setting needs to see.
            if (auto loaded = _config.Load(VoltMod::AddonFile(AddonName, "configs/settings.jsonc")); !loaded)
                return Reply{std::format("Settings not reloaded: {}", loaded.error().Detail)};
            // Keep valid rules active if the edited file cannot be parsed.
            if (auto loaded = _detections.Load(DetectionDataPath); !loaded)
                Log::Warn("{} could not be re-read ({}); keeping the tables already loaded.", DetectionDataPath,
                          loaded.error().Detail);
            else
                LoadDetectionData();
            _cores.RefreshTeamRules();
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
    const Detectors& cores = _cores;
    bool any = false;
    for (const VoltMod::Player* player : _rt.Players.All())
    {
        const int slot = player ? player->Slot() : -1;
        if (!InSlotRange(slot) || (player->IsBot() && !cores.IncludesBots()))
            continue;
        any = true;

        std::string latched;
        const std::span<const CvarRule> rules = cores.InvalidCvars.Rules().All();
        for (size_t index = 0; index < rules.size(); ++index)
        {
            if (!cores.InvalidCvars.IsLatchedAt(slot, index))
                continue;
            if (!latched.empty())
                latched += ",";
            latched += rules[index].name;
        }

        report.push_back(std::format(
            "[AC] s{} {} ({}) punished={} aimbot={} aimlock={}{} antiaim={:.1f} silentaim={} trigger={} recoil={}{} "
            "mouse={}{} wallhack={}{} names={} cvars=[{}] pending={} poll={:.1f}s shots={} cmds={} gen={}",
            slot, player->Name(), player->SteamId(), PunishmentName(_response.Issued(slot)),
            cores.Aimbot.IncidentCount(slot), cores.Aimlock.IncidentCount(slot),
            cores.Aimlock.IsTracking(slot) ? "/tracking" : "", cores.AntiAim.Score(slot),
            cores.SilentAim.Score(slot, now), cores.Triggerbot.Score(slot, now), cores.Recoil.Score(slot, now),
            cores.Recoil.InSpray(slot) ? "/spraying" : "", cores.Mouse.Score(slot, now),
            cores.Mouse.Calibrated(slot) ? "" : "/uncalibrated", cores.Wallhack.Score(slot, now),
            cores.Wallhack.IsTracking(slot) ? "/tracking" : "", cores.Namechanger.ChangeCount(slot),
            latched.empty() ? "-" : latched, _rt.Hooks.ClientConVars.PendingCount(slot),
            _invalidCvarPoller.PollsIn(slot, now), cores.Correlator.Shots(slot).size(),
            cores.Correlator.CommandCount(slot), cores.Correlator.Generation(slot)));
    }
    if (!any)
        report.push_back(cores.IncludesBots() ? "[AC] no players connected." : "[AC] no human players connected.");
    return report;
}

}  // namespace Anticheat
