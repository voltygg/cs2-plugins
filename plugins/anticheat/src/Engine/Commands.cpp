#include "Engine/Commands.hpp"

#include "Engine/StatusReport.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Time.hpp>
#include <format>
#include <string>

using VoltMod::Caller;
using VoltMod::Reply;
using VoltMod::Result;
using VoltMod::Time;

namespace Log = VoltMod::Log;

namespace Anticheat
{

void RegisterCommands(App& app)
{
    auto& commands = app.Runtime.Commands;

    commands.Add("anticheat_reload")
        .Describe("Re-read settings.jsonc and detections.jsonc, and drop all accumulated evidence.")
        .ConsoleOnly()
        .Run([&app](Caller) -> Result<Reply> {
            // The reason names the offending key and its position, which is what a mistyped setting needs.
            if (auto loaded = app.Config.Load(VoltMod::AddonFile(AddonName, "configs/settings.jsonc")); !loaded)
                return Reply{std::format("Settings not reloaded: {}", loaded.error().Detail)};
            // Keep valid rules active if the edited file cannot be parsed.
            if (auto loaded = app.RuleTables.Load(DetectionDataPath); !loaded)
                Log::Warn("{} could not be re-read ({}); keeping the tables already loaded.", DetectionDataPath,
                          loaded.error().Detail);
            else
                app.LoadDetectionData();
            app.Detection.RefreshTeamRules();
            app.ResetEvidence();
            return Reply{std::format("Settings reloaded (mode={}); evidence cleared.", app.Config.Get().anticheat.mode)};
        });

    commands.Add("anticheat_status")
        .Describe("Print the rule state and per-player detection evidence.")
        .ConsoleOnly()
        .Run([&app](Caller caller) -> Result<Reply> {
            // One line per call, so a remote console that keeps the first line still sees them all.
            for (const std::string& line : StatusLines(app, Time::MonotonicSeconds()))
                caller.SayRaw(line);
            return Reply::Silent();
        });
}

}  // namespace Anticheat
