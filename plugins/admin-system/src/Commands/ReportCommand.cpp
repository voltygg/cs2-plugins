#include "../Core/Managers.hpp"
#include "../Reports/ReportFlow.hpp"
#include "../Reports/ReportManager.hpp"

#include <CS2Kit/Api.hpp>
#include <string>

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using AdminSystem::Reports::ReportDenial;
using CS2Kit::Registry;

namespace
{

const bool _registered = [] {
    Registry<CommandSpec>::Add({
        .Name = "report",
        .Aliases = {"r"},
        .Description = "Report a player to the server staff.",
        .Usage = "!report",
        // No .Permission - reporting is open to every player. No .Args either: the menu is the
        // only entry point, so a typed target or reason is deliberately unsupported.
        .Handler =
            [](CommandContext& c) {
                // Fail before opening a menu; the flow re-runs the full gate before it writes.
                const auto gate = App().Reports.CanReport(c.Caller->GetSteamID());
                switch (gate.Reason)
                {
                case ReportDenial::Disabled:
                    return c.Fail("report.disabled");
                case ReportDenial::OnCooldown:
                    return c.Fail("report.cooldown", {{"seconds", std::to_string(gate.SecondsLeft)}});
                default:
                    break;
                }

                AdminSystem::Reports::OpenReportMenu(c.CallerSlot());
                return CommandResult{true, ""};  // the menu is the feedback; no chat reply
            },
    });

    return true;
}();

}  // namespace

}  // namespace AdminSystem::Commands
