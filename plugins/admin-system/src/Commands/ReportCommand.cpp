#include "../Core/App.hpp"
#include "../Reports/ReportFlow.hpp"
#include "../Reports/ReportManager.hpp"
#include "Commands.hpp"

#include <CS2Kit/Api.hpp>
#include <string>

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using AdminSystem::Reports::ReportDenial;

void RegisterReportCommand(CS2Kit::CommandManager& commands, App& app)
{
    commands.Register({
        .Name = "report",
        .Aliases = {"r"},
        .Description = "Report a player to the server staff.",
        .Usage = "!report",
        // No .Permission - reporting is open to every player. No .Args either: the menu is the
        // only entry point, so a typed target or reason is deliberately unsupported.
        .Handler =
            [&app](CommandContext& c) {
                // Fail before opening a menu; the flow re-runs the full gate before it writes.
                const auto gate = app.Reports.CanReport(c.Caller->GetSteamID());
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
                return CommandResult{true, ""};  // the menu is the feedback; no chat reply
            },
    });
}

}  // namespace AdminSystem::Commands
