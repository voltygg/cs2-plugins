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

/** An empty field reads as a dash, so the columns stay where the eye expects them. */
static std::string OrDash(std::string text)
{
    return text.empty() ? "-" : std::move(text);
}

/** Episodes and calibration in progress right now - state a score cannot show. */
static std::string InProgress(const Detectors& detectors, int slot)
{
    std::string flags;
    const auto add = [&flags](bool active, std::string_view name) {
        if (!active)
            return;
        if (!flags.empty())
            flags += ",";
        flags += name;
    };
    add(detectors.Aimlock.IsTracking(slot), "aimlock");
    add(detectors.Wallhack.IsTracking(slot), "wallhack");
    add(detectors.Recoil.InSpray(slot), "spray");
    add(!detectors.Mouse.Calibrated(slot), "mouse-learning");
    return OrDash(std::move(flags));
}

/** The cvars this player has already been reported for. */
static std::string ReportedCvars(const Detectors& detectors, int slot)
{
    std::string names;
    const std::span<const CvarRule> rules = detectors.InvalidCvars.Rules().All();
    for (size_t index = 0; index < rules.size(); ++index)
    {
        if (!detectors.InvalidCvars.AlreadyReportedAt(slot, index))
            continue;
        if (!names.empty())
            names += ",";
        names += rules[index].name;
    }
    return OrDash(std::move(names));
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

        report.push_back(std::format(
            "[AC] s{} {} ({}) punished={} suspicion={:.2f} ({}) active={} cvars=[{}] queries={} poll={:.1f}s "
            "shots={} cmds={} gen={}",
            slot, player->Name(), player->SteamId(), PunishmentName(app.Response.Issued(player->SteamId())),
            detectors.Scores.Total(slot, nowSec), OrDash(detectors.Scores.Breakdown(slot, nowSec)),
            InProgress(detectors, slot), ReportedCvars(detectors, slot),
            app.Runtime.Hooks.ClientConVars.PendingCount(slot), app.Cvars.PollsIn(slot, nowSec),
            detectors.History.Shots(slot).size(), detectors.History.CommandCount(slot),
            detectors.History.Generation(slot)));
    }
    if (!any)
        report.push_back(detectors.IncludesBots() ? "[AC] no players connected." : "[AC] no human players connected.");
    return report;
}

}  // namespace Anticheat
