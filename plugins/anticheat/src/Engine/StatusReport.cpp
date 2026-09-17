#include "Engine/StatusReport.hpp"

#include <VoltMod/Core/Json.hpp>
#include <VoltMod/Core/Slot.hpp>
#include <format>
#include <map>
#include <span>
#include <string>

namespace Anticheat
{

std::string StatusJson(const App& app)
{
    const auto& settings = app.Config.Get().anticheat;
    std::map<std::string, bool> rules;
    for (const DetectionInfo& detection : DetectionCatalog)
        rules.emplace(detection.Token, app.Detection.RuleEnabled(detection.Kind));

    const VoltMod::Status sight = app.Feed.SightAvailable();
    return VoltMod::Json::Write(
        glz::obj{"enabled",
                 settings.enabled,
                 "mode",
                 ModeName(app.Response.CurrentMode()),
                 "detecting",
                 app.Detection.Enabled(),
                 "enforcingCheatCvars",
                 app.Detection.EnforceCheatCvars(),
                 "rules",
                 rules,
                 "clientCvars",
                 app.Runtime.Hooks.ClientConVars.Available() ? "available" : "degraded",
                 "teleportTracker",
                 app.Runtime.Hooks.Teleport.Available().has_value(),
                 "sightLines",
                 sight ? std::string("available") : sight.error().Detail,
                 "shotHistoryFrames",
                 app.Detection.History.FrameCount(),
                 "detectionData",
                 glz::obj{"cvarRules", app.Detection.InvalidCvars.Rules().Size(), "blacklistedEvents",
                          app.RuleTables.Get().dllEventBlacklist.size()},
                 "webhook",
                 !settings.webhook.url.empty(),
                 "simulator",
                 settings.debug.simulator,
                 "includeBots",
                 settings.debug.includeBots});
}

std::vector<std::string> StatusLines(const App& app, double nowSec)
{
    std::vector<std::string> report{std::format("[AC] {}", StatusJson(app))};

    const Detectors& detectors = app.Detection;
    bool any = false;
    for (const VoltMod::Player* player : app.Runtime.Players.All())
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
            slot, player->Name(), player->SteamId(), PunishmentName(app.Response.Issued(slot)),
            detectors.Aimbot.IncidentCount(slot), detectors.Aimlock.IncidentCount(slot),
            detectors.Aimlock.IsTracking(slot) ? "/tracking" : "", detectors.AntiAim.Score(slot),
            detectors.SilentAim.Score(slot, nowSec), detectors.Triggerbot.Score(slot, nowSec),
            detectors.Recoil.Score(slot, nowSec), detectors.Recoil.InSpray(slot) ? "/spraying" : "",
            detectors.Mouse.Score(slot, nowSec), detectors.Mouse.Calibrated(slot) ? "" : "/uncalibrated",
            detectors.Wallhack.Score(slot, nowSec), detectors.Wallhack.IsTracking(slot) ? "/tracking" : "",
            detectors.Namechanger.ChangeCount(slot), latched.empty() ? "-" : latched,
            app.Runtime.Hooks.ClientConVars.PendingCount(slot), app.Cvars.PollsIn(slot, nowSec),
            detectors.History.Shots(slot).size(), detectors.History.CommandCount(slot),
            detectors.History.Generation(slot)));
    }
    if (!any)
        report.push_back(detectors.IncludesBots() ? "[AC] no players connected." : "[AC] no human players connected.");
    return report;
}

}  // namespace Anticheat
