#include "IssuePunishment.hpp"

#include "../Admin/FreezeManager.hpp"
#include "../Core/App.hpp"
#include "../Core/ChatService.hpp"
#include "PunishmentManager.hpp"

#include <VoltMod/Api.hpp>
#include <format>

using AdminSystem::Database::Ban;
using AdminSystem::Database::TextMute;
using AdminSystem::Database::VoiceMute;

namespace AdminSystem::Punishments
{

using VoltMod::Player;

/** Fill the common target/admin/reason fields, plus Duration when the entity has one (Warning does not). */
template <typename T>
static void Fill(T& punishment, const Player& target, const Player& admin, const std::string& reason,
                 int64_t durationSec)
{
    punishment.TargetSteamId = target.SteamId();
    punishment.TargetName = target.Name();
    punishment.AdminSteamId = admin.SteamId();
    punishment.AdminName = admin.Name();
    punishment.Reason = reason;

    if constexpr (requires { punishment.Duration = durationSec; })
    {
        punishment.Duration = durationSec;
    }
}

static bool Issue(App& app, const Player& admin, const Player& target, PunishType type, const std::string& reason,
                  int64_t durationSec)
{
    auto& pm = app.Punishments;
    switch (type)
    {
    case PunishType::Kick:
    {
        (void)app.Runtime.Entities.Controller(target.Slot()).Kick(reason);
        app.Chat.BroadcastPunishment("kicked", admin.Name(), target.Name(), reason, 0);
        return true;
    }
    case PunishType::Ban:
    {
        Ban ban;
        Fill(ban, target, admin, reason, durationSec);
        ban.TargetIp = target.Ip();
        return pm.IssueBan(ban);
    }
    case PunishType::VoiceMute:
    {
        VoiceMute mute;
        Fill(mute, target, admin, reason, durationSec);
        return pm.IssueVoiceMute(mute);
    }
    case PunishType::TextMute:
    {
        TextMute mute;
        Fill(mute, target, admin, reason, durationSec);
        return pm.IssueTextMute(mute);
    }
    case PunishType::Warn:
    {
        Database::Warning warn;
        Fill(warn, target, admin, reason, 0);
        return pm.IssueWarning(warn);
    }
    }
    return false;
}

bool IssuePunishment(App& app, const Player& admin, const Player& target, PunishType type, const std::string& reason,
                     int64_t durationSec)
{
    // Capture identity up front: a kick invalidates `target` before the audit write below.
    int64_t targetSteamId = target.SteamId();
    std::string targetName = target.Name();

    if (!Issue(app, admin, target, type, reason, durationSec))
        return false;

    // Audit + abuse-rate check. Covers chat commands and the menu (both land here); the
    // warning->ban auto-escalation calls PunishmentManager directly and is deliberately
    // not counted against the admin.
    auto detail = durationSec > 0 ? std::format("{}; {}s", reason, durationSec) : reason;
    app.Freeze.RecordPunishment(admin.SteamId(), admin.Name(), AuditActionName(type), targetSteamId, targetName,
                                detail);
    return true;
}

}  // namespace AdminSystem::Punishments
