#include "IssuePunishment.hpp"

#include "../Admin/FreezeManager.hpp"
#include "../Core/App.hpp"
#include "../Core/ChatService.hpp"
#include "PunishmentManager.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Sdk/Entity/PlayerController.hpp>
#include <format>

namespace AdminSystem::Punishments
{

using namespace AdminSystem::Database;
using VoltMod::Players::Player;

namespace
{

/** Fill the common target/admin/reason fields, plus Duration when the entity has one (Warning does not). */
template <typename T>
void Fill(T& punishment, const Player& target, const Player& admin, const std::string& reason, int64_t durationSec)
{
    punishment.TargetSteamId = target.GetSteamID();
    punishment.TargetName = target.GetName();
    punishment.AdminSteamId = admin.GetSteamID();
    punishment.AdminName = admin.GetName();
    punishment.Reason = reason;

    if constexpr (requires { punishment.Duration = durationSec; })
    {
        punishment.Duration = durationSec;
    }
}

bool Issue(App& app, const Player& admin, const Player& target, PunishType type, const std::string& reason,
           int64_t durationSec)
{
    auto& pm = app.Punishments;
    switch (type)
    {
    case PunishType::Kick:
    {
        VoltMod::PlayerController(target.GetSlot()).Kick(reason.c_str());
        app.Chat.BroadcastPunishment("kicked", admin.GetName(), target.GetName(), reason, 0);
        return true;
    }
    case PunishType::Ban:
    {
        Ban ban;
        Fill(ban, target, admin, reason, durationSec);
        ban.TargetIp = target.GetIpAddress();
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
        Warning warn;
        Fill(warn, target, admin, reason, 0);
        return pm.IssueWarning(warn);
    }
    }
    return false;
}

}  // namespace

bool IssuePunishment(App& app, const Player& admin, const Player& target, PunishType type, const std::string& reason,
                     int64_t durationSec)
{
    // Capture identity up front: a kick invalidates `target` before the audit write below.
    int64_t targetSteamId = target.GetSteamID();
    std::string targetName = target.GetName();

    if (!Issue(app, admin, target, type, reason, durationSec))
        return false;

    // Audit + abuse-rate check. Covers chat commands and the menu (both land here); the
    // warning->ban auto-escalation calls PunishmentManager directly and is deliberately
    // not counted against the admin.
    auto detail = durationSec > 0 ? std::format("{}; {}s", reason, durationSec) : reason;
    app.Freeze.RecordPunishment(admin.GetSteamID(), admin.GetName(), AuditActionName(type), targetSteamId, targetName,
                                detail);
    return true;
}

}  // namespace AdminSystem::Punishments
