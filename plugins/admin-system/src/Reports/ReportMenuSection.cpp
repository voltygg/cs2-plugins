#include "Reports/ReportMenuSection.hpp"

#include "App.hpp"
#include "Reports/ReportFlow.hpp"
#include "Reports/ReportManager.hpp"

#include <VoltMod/Api.hpp>
#include <string>

namespace AdminSystem::Reports
{

void ReportMenuSection::Publish()
{
    _app.Runtime.Exchange.Publish<Contracts::IMenuSection>(this, "report");
}

void ReportMenuSection::Unpublish()
{
    _app.Runtime.Exchange.Unpublish<Contracts::IMenuSection>("report");
}

bool ReportMenuSection::IsVisibleTo(int slot)
{
    return _app.Runtime.Players.Get(slot) && _app.Settings.Get().reports.enabled;
}

bool ReportMenuSection::Open(int slot)
{
    const VoltMod::Player* player = _app.Runtime.Players.Get(slot);
    if (!player)
    {
        return false;
    }

    // The same gate as `!report`, so the menu entry and the command refuse alike.
    const ReportGate gate = _app.Reports.CanReport(player->SteamId());
    if (gate.Reason == ReportDenial::OnCooldown)
    {
        _app.Runtime.Messages.ReplyKey(slot, "report.cooldown", {{"seconds", std::to_string(gate.SecondsLeft)}});
        return true;
    }
    if (!gate)
    {
        return false;
    }

    OpenReportMenu(_app, slot);
    return true;
}

}  // namespace AdminSystem::Reports
