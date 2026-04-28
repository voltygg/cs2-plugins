#include "PunishmentManager.hpp"

#include <CS2Kit/Players/PlayerManager.hpp>

namespace AdminSystem::Punishments
{

using namespace CS2Kit::Players;

bool PunishmentManager::LoadActivePunishments()
{
    try
    {
        // TODO: Use new BanRepository to load active bans
        // auto& banRepo = Database::BanRepository::Instance();
        // auto bans = banRepo.FindAllActive();

        _activeBans.clear();
        // for (const auto& ban : bans)
        // {
        //     _activeBans[ban.TargetSteamId] = ban;
        // }

        // TODO: Load mutes and gags

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

std::optional<Database::Ban> PunishmentManager::GetActiveBan(int64_t steamId)
{
    auto it = _activeBans.find(steamId);
    if (it != _activeBans.end())
    {
        // Check if expired
        if (!it->second.IsExpired())
        {
            return it->second;
        }

        // Remove expired ban
        _activeBans.erase(it);
    }

    return std::nullopt;
}

bool PunishmentManager::IsMuted(int64_t steamId)
{
    return _mutedPlayers.count(steamId) > 0;
}

bool PunishmentManager::IsGagged(int64_t steamId)
{
    return _gaggedPlayers.count(steamId) > 0;
}

bool PunishmentManager::IssueBan(const Database::Ban& ban)
{
    try
    {
        // TODO: Save to database via BanRepository
        // Database::BanRepository repo;
        // if (!repo.Create(ban))
        //     return false;

        // Add to cache
        _activeBans[ban.TargetSteamId] = ban;

        // Kick player if online
        auto& playerMgr = PlayerManager::Instance();
        auto* player = playerMgr.GetPlayerBySteamId(ban.TargetSteamId);
        if (player)
        {
            // TODO: Kick player from server
        }

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool PunishmentManager::IssueMute(const Database::Mute& mute)
{
    try
    {
        // TODO: Save to database via MuteRepository

        // Add to cache
        _mutedPlayers.insert(mute.TargetSteamId);

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool PunishmentManager::IssueGag(const Database::Gag& gag)
{
    try
    {
        // TODO: Save to database via GagRepository

        // Add to cache
        _gaggedPlayers.insert(gag.TargetSteamId);

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool PunishmentManager::IssueWarning(const Database::Warning& warning)
{
    // TODO: Save to database via WarningRepository
    // TODO: Check warning count and auto-punish if threshold reached
    return true;
}

bool PunishmentManager::RemoveBan(int64_t banId, int64_t removedBy, const std::string& reason)
{
    try
    {
        // TODO: Remove from database via BanRepository
        // Database::BanRepository repo;
        // if (!repo.Remove(banId, removedBy, reason))
        //     return false;

        // Remove from cache (find by ban ID)
        for (auto it = _activeBans.begin(); it != _activeBans.end(); ++it)
        {
            if (it->second.Id == banId)
            {
                _activeBans.erase(it);
                break;
            }
        }

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool PunishmentManager::RemoveMute(int64_t muteId, int64_t removedBy, const std::string& reason)
{
    // TODO: Remove from database via MuteRepository
    // TODO: Remove from cache (need to track mute ID -> SteamID mapping)
    return true;
}

bool PunishmentManager::RemoveGag(int64_t gagId, int64_t removedBy, const std::string& reason)
{
    // TODO: Remove from database via GagRepository
    // TODO: Remove from cache
    return true;
}

void PunishmentManager::ExpireOldPunishments()
{
    // TODO: Expire bans in database
    // Database::BanRepository banRepo;
    // banRepo.ExpireOldBans();

    // Remove expired from cache
    for (auto it = _activeBans.begin(); it != _activeBans.end();)
    {
        if (it->second.IsExpired())
        {
            it = _activeBans.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // TODO: Expire mutes and gags
}

}  // namespace AdminSystem::Punishments
