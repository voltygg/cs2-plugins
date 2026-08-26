#include "../Core/App.hpp"
#include "../Reports/ReportFlow.hpp"
#include "../Reports/ReportManager.hpp"
#include "Commands.hpp"

#include <VoltMod/Api.hpp>
#include <string>

using VoltMod::CommandContext;
using VoltMod::CommandResult;

namespace AdminSystem::Commands
{

using AdminSystem::Reports::ReportDenial;

void RegisterReportCommand(VoltMod::CommandManager& commands, App& app)
{
    commands.Register({
        .Name = "report",
        .Aliases = {"r"},
        .Description = "Report a player to the server staff.",
        // No .Permission - reporting is open to every player. No .Args either: the menu is the
        // only entry point, so a typed target or reason is deliberately unsupported.
        .Handler =
            [&app](CommandContext& c) {
                // Fail before opening a menu; the flow re-runs the full gate before it writes.
                const auto gate = app.Reports.CanReport(c.Caller->SteamId());
                switch (gate.Reason)
                {
                case ReportDenial::Disabled:
                    return c.Fail("report.disabled");
                case ReportDenial::OnCooldown:
                    return c.Fail("report.cooldown", {{"seconds", std::to_string(gate.SecondsLeft)}});
                default:
                    break;
                }

                AdminSystem::Reports::OpenReportMenu(app, c.CallerSlot());
                return CommandResult::Silent();
            },
    });
}

}  // namespace AdminSystem::Commands
