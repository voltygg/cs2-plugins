#include "FreezeManager.hpp"

#include "../Core/ChatService.hpp"
#include "../Core/Config.hpp"
#include "../Core/Managers.hpp"
#include "../Core/Permissions.hpp"
#include "../Database/Repositories/AdminActivityRepository.hpp"
#include "AdminManager.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/UserMessage.hpp>
#include <CS2Kit/Utils/ChatColors.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>

namespace AdminSystem::Admin
{

namespace Db = AdminSystem::Database;
namespace Log = CS2Kit::Utils::Log;
namespace ChatColors = CS2Kit::Utils::ChatColors;
using CS2Kit::Core::Engine;
using CS2Kit::Utils::TimeUtils;

void FreezeManager::RefreshFromDatabase()
{
    // Async poll; on DB failure the callback never runs and the cached set is kept, so
    // nobody unfreezes by accident.
    Db::AdminRepository{}.FindFrozenAsync([this](std::vector<Db::FrozenAdmin> rows) {
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
    App().Chat.BroadcastAction("broadcast.frozeAdmin", byName, targetName);
    return true;
}

bool FreezeManager::Unfreeze(int64_t targetSteamId, int64_t bySteamId, const std::string& byName)
{
    auto it = _frozen.find(targetSteamId);
    if (it == _frozen.end())
        return false;

    if (!Db::AdminRepository{}.ClearFrozen(targetSteamId))
        return false;

    std::string targetName = it->second.Name;
    _frozen.erase(it);

    RecordAudit(bySteamId, byName, "unfreeze_admin", targetSteamId, targetName, "");
    Log::Info("Admin {} ({}) unfrozen by {} ({}).", targetName, targetSteamId, byName, bySteamId);
    App().Chat.BroadcastAction("broadcast.unfrozeAdmin", byName, targetName);
    return true;
}

void FreezeManager::RecordPunishment(int64_t adminSteamId, const std::string& adminName, const std::string& action,
                                     int64_t targetSteamId, const std::string& targetName, const std::string& detail)
{
    RecordAudit(adminSteamId, adminName, action, targetSteamId, targetName, detail);

    // Console actions and already-frozen admins never trip the rate check; root admins are
    // exempt by design (they resolve every flag, including 'z' itself).
    if (adminSteamId == 0 || !App().Config.GetAbuseProtection().enabled || IsFrozen(adminSteamId))
        return;
    if (App().Admins.HasPermission(adminSteamId, Permission::Root))
        return;

    CheckAutoFreeze(adminSteamId, adminName);
}

void FreezeManager::RecordAudit(int64_t adminSteamId, const std::string& adminName, const std::string& action,
                                int64_t targetSteamId, const std::string& targetName, const std::string& detail)
{
    Db::AdminActivityRepository{}.Record(adminSteamId, adminName, action, targetSteamId, targetName, detail,
                                         App().Config.GetServer().tag);
}

bool FreezeManager::ApplyFreeze(int64_t steamId, const std::string& name, int64_t bySteamId, const std::string& byName,
                                const std::string& reason)
{
    if (!Db::AdminRepository{}.SetFrozen(steamId, bySteamId, reason))
        return false;

    _frozen[steamId] = {
        .SteamId = steamId, .Name = name, .FrozenAt = TimeUtils::Now(), .FrozenBy = bySteamId, .Reason = reason};

    RecordAudit(bySteamId, byName, "freeze_admin", steamId, name, reason);
    NotifyFrozen(steamId);
    return true;
}

void FreezeManager::CheckAutoFreeze(int64_t adminSteamId, const std::string& adminName)
{
    const auto& cfg = App().Config.GetAbuseProtection();
    int64_t windowStart = TimeUtils::Now() - static_cast<int64_t>(cfg.windowMinutes) * 60;

    // FIFO on the worker: this count sees the audit insert that triggered the check.
    Db::AdminActivityRepository{}.CountSinceAsync(
        adminSteamId, windowStart, [this, adminSteamId, adminName](Db::ActivityCounts counts) {
            const auto& limits = App().Config.GetAbuseProtection();
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
            App().Chat.BroadcastAction("broadcast.autoFrozeAdmin", adminName, "");
        });
}

void FreezeManager::NotifyFrozen(int64_t steamId)
{
    auto* player = Engine().Players.GetPlayerBySteamId(steamId);
    if (!player)
        return;

    const auto* row = GetFrozen(steamId);
    int slot = player->GetSlot();
    auto notice = Engine().Translations.Get("freeze.notice", slot, {{"reason", row ? row->Reason : ""}});
    Engine().Messages.Reply(slot, std::format("{}{}", ChatColors::Red, notice));
}

}  // namespace AdminSystem::Admin
