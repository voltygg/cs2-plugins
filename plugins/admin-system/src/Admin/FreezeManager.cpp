#include "FreezeManager.hpp"

#include "../Core/App.hpp"
#include "../Core/ChatService.hpp"
#include "../Core/Config.hpp"
#include "../Core/Permissions.hpp"
#include "../Database/Repositories/AdminActivityRepository.hpp"
#include "AdminManager.hpp"

#include <CS2Kit/Core/ChatColors.hpp>
#include <CS2Kit/Core/Log.hpp>
#include <CS2Kit/Core/TimeUtils.hpp>
#include <CS2Kit/Core/Translations.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Runtime.hpp>
#include <CS2Kit/Sdk/UserMessage.hpp>
#include <format>

namespace AdminSystem::Admin
{

namespace Db = AdminSystem::Database;
namespace Log = CS2Kit::Core::Log;
namespace ChatColors = CS2Kit::Core::ChatColors;
using CS2Kit::Core::TimeUtils;

void FreezeManager::RefreshFromDatabase()
{
    // Async poll; on DB failure the callback never runs and the cached set is kept, so
    // nobody unfreezes by accident.
    Db::AdminRepository{_app.Db}.FindFrozenAsync([this](std::vector<Db::FrozenAdmin> rows) {
        std::unordered_map<int64_t, Db::FrozenAdmin> fresh;
        for (auto& row : rows)
            fresh.emplace(row.SteamId, std::move(row));

        for (const auto& [steamId, row] : fresh)
        {
            if (!_frozen.contains(steamId))
                NotifyFrozen(steamId);
        }

        _frozen = std::move(fresh);
    });
}

const Db::FrozenAdmin* FreezeManager::GetFrozen(int64_t steamId) const
{
    auto it = _frozen.find(steamId);
    return it != _frozen.end() ? &it->second : nullptr;
}

bool FreezeManager::Freeze(int64_t targetSteamId, const std::string& targetName, int64_t bySteamId,
                           const std::string& byName, const std::string& reason)
{
    if (!ApplyFreeze(targetSteamId, targetName, bySteamId, byName, reason))
        return false;

    Log::Warn("Admin {} ({}) frozen by {} ({}): {}", targetName, targetSteamId, byName, bySteamId, reason);
    _app.Chat.BroadcastAction("broadcast.frozeAdmin", byName, targetName);
    return true;
}

bool FreezeManager::Unfreeze(int64_t targetSteamId, int64_t bySteamId, const std::string& byName)
{
    auto it = _frozen.find(targetSteamId);
    if (it == _frozen.end())
        return false;

    if (!Db::AdminRepository{_app.Db}.ClearFrozen(targetSteamId))
        return false;

    std::string targetName = it->second.Name;
    _frozen.erase(it);

    RecordAudit(bySteamId, byName, "unfreeze_admin", targetSteamId, targetName, "");
    Log::Info("Admin {} ({}) unfrozen by {} ({}).", targetName, targetSteamId, byName, bySteamId);
    _app.Chat.BroadcastAction("broadcast.unfrozeAdmin", byName, targetName);
    return true;
}

void FreezeManager::RecordPunishment(int64_t adminSteamId, const std::string& adminName, const std::string& action,
                                     int64_t targetSteamId, const std::string& targetName, const std::string& detail)
{
    RecordAudit(adminSteamId, adminName, action, targetSteamId, targetName, detail);

    // Console actions and already-frozen admins never trip the rate check; root admins are
    // exempt by design (they resolve every flag, including 'z' itself).
    if (adminSteamId == 0 || !_app.Config.GetAbuseProtection().enabled || IsFrozen(adminSteamId))
        return;
    if (_app.Admins.HasPermission(adminSteamId, Permission::Root))
        return;

    CheckAutoFreeze(adminSteamId, adminName);
}

void FreezeManager::RecordAudit(int64_t adminSteamId, const std::string& adminName, const std::string& action,
                                int64_t targetSteamId, const std::string& targetName, const std::string& detail)
{
    Db::AdminActivityRepository{_app.Db}.Record(adminSteamId, adminName, action, targetSteamId, targetName, detail,
                                                _app.Config.GetServer().tag);
}

bool FreezeManager::ApplyFreeze(int64_t steamId, const std::string& name, int64_t bySteamId, const std::string& byName,
                                const std::string& reason)
{
    if (!Db::AdminRepository{_app.Db}.SetFrozen(steamId, bySteamId, reason))
        return false;

    _frozen[steamId] = {
        .SteamId = steamId, .Name = name, .FrozenAt = TimeUtils::Now(), .FrozenBy = bySteamId, .Reason = reason};

    RecordAudit(bySteamId, byName, "freeze_admin", steamId, name, reason);
    NotifyFrozen(steamId);
    return true;
}

void FreezeManager::CheckAutoFreeze(int64_t adminSteamId, const std::string& adminName)
{
    const auto& cfg = _app.Config.GetAbuseProtection();
    int64_t windowStart = TimeUtils::Now() - static_cast<int64_t>(cfg.windowMinutes) * 60;

    // FIFO on the worker: this count sees the audit insert that triggered the check.
    Db::AdminActivityRepository{_app.Db}.CountSinceAsync(
        adminSteamId, windowStart, [this, adminSteamId, adminName](Db::ActivityCounts counts) {
            const auto& limits = _app.Config.GetAbuseProtection();
            bool tripped = (limits.maxBans > 0 && counts.Bans >= limits.maxBans) ||
                           (limits.maxKicks > 0 && counts.Kicks >= limits.maxKicks) ||
                           (limits.maxMutes > 0 && counts.Mutes >= limits.maxMutes) ||
                           (limits.maxWarnings > 0 && counts.Warnings >= limits.maxWarnings);
            if (!tripped || IsFrozen(adminSteamId))
                return;

            auto reason = std::format("Rate limit exceeded: {} bans, {} kicks, {} mutes, {} warnings in {} min",
                                      counts.Bans, counts.Kicks, counts.Mutes, counts.Warnings, limits.windowMinutes);
            if (!ApplyFreeze(adminSteamId, adminName, 0, "", reason))
            {
                Log::Error("Auto-freeze of {} ({}) failed to persist: {}", adminName, adminSteamId, reason);
                return;
            }

            Log::Warn("AUTO-FROZE admin {} ({}): {}", adminName, adminSteamId, reason);
            _app.Chat.BroadcastAction("broadcast.autoFrozeAdmin", adminName, "");
        });
}

void FreezeManager::NotifyFrozen(int64_t steamId)
{
    auto* player = _app.Runtime.Players.GetPlayerBySteamId(steamId);
    if (!player)
        return;

    const auto* row = GetFrozen(steamId);
    int slot = player->GetSlot();
    auto notice = _app.Runtime.Translations.Get("freeze.notice", slot, {{"reason", row ? row->Reason : ""}});
    _app.Runtime.Messages.Reply(slot, std::format("{}{}", ChatColors::Red, notice));
}

}  // namespace AdminSystem::Admin
