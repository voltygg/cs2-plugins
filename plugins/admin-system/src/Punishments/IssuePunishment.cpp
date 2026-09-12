#include "IssuePunishment.hpp"

#include "../Admin/FreezeManager.hpp"
#include "../Core/App.hpp"
#include "../Core/ChatService.hpp"
#include "PunishmentManager.hpp"

#include <VoltMod/Api.hpp>
#include <format>

namespace AdminSystem::Punishments
{

using Database::Punishment;
using VoltMod::Player;

static bool Issue(App& app, const Player& admin, const Player& target, PunishType type, const std::string& reason,
                  int64_t durationSec)
{
    // A kick leaves no row; it is applied and broadcast here.
    if (type == PunishType::Kick)
    {
        (void)app.Runtime.Entities.Controller(target.Slot()).Kick(reason);
        app.Chat.BroadcastPunishment("kicked", admin.Name(), target.Name(), reason, 0);
        return true;
    }

    Punishment record{.Kind = type,
                      .TargetSteamId = target.SteamId(),
                      .TargetName = std::string(target.Name()),
                      .TargetIp = type == PunishType::Ban ? std::string(target.Ip()) : std::string{},
                      .AdminSteamId = admin.SteamId(),
                      .AdminName = std::string(admin.Name()),
                      .Reason = reason,
                      .Duration = IsTimed(type) ? durationSec : 0};
    return app.Punishments.Issue(record);
}

bool IssuePunishment(App& app, const Player& admin, const Player& target, PunishType type, const std::string& reason,
                     int64_t durationSec)
{
    // Capture identity up front: a kick invalidates `target` before the audit write below.
    int64_t targetSteamId = target.SteamId();
    std::string targetName = target.Name();

    if (!Issue(app, admin, target, type, reason, durationSec))
        return false;

    // Audit + abuse-rate check for chat commands and the menu. The warning->ban escalation goes
    // through PunishmentManager instead, so it is deliberately not counted against the admin.
    auto detail = durationSec > 0 ? std::format("{}; {}s", reason, durationSec) : reason;
    app.Freeze.RecordPunishment(admin.SteamId(), admin.Name(), AuditActionName(type), targetSteamId, targetName,
                                detail);
    return true;
}

}  // namespace AdminSystem::Punishments
