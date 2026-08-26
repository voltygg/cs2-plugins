#include "FreezeManager.hpp"

#include "../Core/ChatService.hpp"
#include "../Core/Config.hpp"
#include "../Core/Permissions.hpp"
#include "../Database/Repositories/AdminActivityRepository.hpp"
#include "AdminManager.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Time.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Messaging/ChatColors.hpp>
#include <VoltMod/Messaging/Messages.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>

namespace AdminSystem::Admin
{

namespace Db = AdminSystem::Database;
namespace Log = VoltMod::Log;
namespace ChatColors = VoltMod::ChatColors;
using VoltMod::Time;

void FreezeManager::RefreshFromDatabase()
{
    // Async poll; on DB failure the callback never runs and the cached set is kept, so
    // nobody unfreezes by accident.
    Db::AdminRepository{_db}.FindFrozenAsync([this](std::vector<Db::FrozenAdmin> rows) {
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

std::optional<Db::FrozenAdmin> FreezeManager::GetFrozen(int64_t steamId) const
{
    auto it = _frozen.find(steamId);
    if (it == _frozen.end())
        return std::nullopt;
    return it->second;
}

bool FreezeManager::Freeze(int64_t targetSteamId, const std::string& targetName, int64_t bySteamId,
                           const std::string& byName, const std::string& reason)
{
    if (!ApplyFreeze(targetSteamId, targetName, bySteamId, byName, reason))
        return false;

    Log::Warn("Admin {} ({}) frozen by {} ({}): {}", targetName, targetSteamId, byName, bySteamId, reason);
    _chat.BroadcastAction("broadcast.frozeAdmin", byName, targetName);
    return true;
}

bool FreezeManager::Unfreeze(int64_t targetSteamId, int64_t bySteamId, const std::string& byName)
{
    auto it = _frozen.find(targetSteamId);
    if (it == _frozen.end())
        return false;

    if (!Db::AdminRepository{_db}.ClearFrozen(targetSteamId))
        return false;

    std::string targetName = it->second.Name;
    _frozen.erase(it);

    RecordAudit(bySteamId, byName, "unfreeze_admin", targetSteamId, targetName, "");
    Log::Info("Admin {} ({}) unfrozen by {} ({}).", targetName, targetSteamId, byName, bySteamId);
    _chat.BroadcastAction("broadcast.unfrozeAdmin", byName, targetName);
    return true;
}

void FreezeManager::RecordPunishment(int64_t adminSteamId, const std::string& adminName, const std::string& action,
                                     int64_t targetSteamId, const std::string& targetName, const std::string& detail)
{
    RecordAudit(adminSteamId, adminName, action, targetSteamId, targetName, detail);

    // Console actions and already-frozen admins never trip the rate check; root admins are
    // exempt by design (they resolve every flag, including 'z' itself).
    if (adminSteamId == 0 || !_config.GetAbuseProtection().enabled || IsFrozen(adminSteamId))
        return;
    // Raw grant, not Access: the frozen case already returned above, and asking the gated
    // surface here would only re-answer that same question.
    if (_admins.HasPermission(adminSteamId, Permission::Root))
        return;

    CheckAutoFreeze(adminSteamId, adminName);
}

void FreezeManager::RecordAudit(int64_t adminSteamId, const std::string& adminName, const std::string& action,
                                int64_t targetSteamId, const std::string& targetName, const std::string& detail)
{
    Db::AdminActivityRepository{_db}.Record(adminSteamId, adminName, action, targetSteamId, targetName, detail,
                                            _config.GetServer().tag);
}

bool FreezeManager::ApplyFreeze(int64_t steamId, const std::string& name, int64_t bySteamId, const std::string& byName,
                                const std::string& reason)
{
    if (!Db::AdminRepository{_db}.SetFrozen(steamId, bySteamId, reason))
        return false;

    _frozen[steamId] = {
        .SteamId = steamId, .Name = name, .FrozenAt = Time::Now(), .FrozenBy = bySteamId, .Reason = reason};

    RecordAudit(bySteamId, byName, "freeze_admin", steamId, name, reason);
    NotifyFrozen(steamId);
    return true;
}

void FreezeManager::CheckAutoFreeze(int64_t adminSteamId, const std::string& adminName)
{
    const auto& cfg = _config.GetAbuseProtection();
    int64_t windowStart = Time::Now() - static_cast<int64_t>(cfg.windowMinutes) * 60;

    // FIFO on the worker: this count sees the audit insert that triggered the check.
    Db::AdminActivityRepository{_db}.CountSinceAsync(
        adminSteamId, windowStart, [this, adminSteamId, adminName](Db::ActivityCounts counts) {
            const auto& limits = _config.GetAbuseProtection();
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
            _chat.BroadcastAction("broadcast.autoFrozeAdmin", adminName, "");
        });
}

void FreezeManager::NotifyFrozen(int64_t steamId)
{
    auto* player = _rt.Players.GetPlayerBySteamId(steamId);
    if (!player)
        return;

    auto row = GetFrozen(steamId);
    int slot = player->GetSlot();
    auto notice = _rt.Translations.Get("freeze.notice", slot, {{"reason", row ? row->Reason : ""}});
    _rt.Messages.Reply(slot, std::format("{}{}", ChatColors::Red, notice));
}

}  // namespace AdminSystem::Admin
