#include "punishment_manager.h"

#include "../database/repositories/ban_repository.h"
#include "../player/player_manager.h"

namespace punishments
{

bool PunishmentManager::LoadActivePunishments()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    try
    {
        database::BanRepository banRepo;
        auto bans = banRepo.FindAllActive();

        m_activeBans.clear();
        for (const auto& ban : bans)
        {
            m_activeBans[ban.target_steam_id] = ban;
        }

        // TODO: Load mutes and gags

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

std::optional<database::Ban> PunishmentManager::GetActiveBan(int64_t steam_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_activeBans.find(steam_id);
    if (it != m_activeBans.end())
    {
        // Check if expired
        if (!it->second.IsExpired())
        {
            return it->second;
        }

        // Remove expired ban
        m_activeBans.erase(it);
    }

    return std::nullopt;
}

bool PunishmentManager::IsMuted(int64_t steam_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_mutedPlayers.contains(steam_id);
}

bool PunishmentManager::IsGagged(int64_t steam_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_gaggedPlayers.contains(steam_id);
}

bool PunishmentManager::IssueBan(const database::Ban& ban)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    try
    {
        // Save to database
        database::BanRepository repo;
        if (!repo.Create(ban))
            return false;

        // Add to cache
        m_activeBans[ban.target_steam_id] = ban;

        // Kick player if online
        auto& playerMgr = player::PlayerManager::Instance();
        auto* player = playerMgr.GetPlayerBySteamID(ban.target_steam_id);
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

bool PunishmentManager::IssueMute(const database::Mute& mute)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    try
    {
        // TODO: Save to database via MuteRepository

        // Add to cache
        m_mutedPlayers.insert(mute.target_steam_id);

        // Update player if online
        auto& playerMgr = player::PlayerManager::Instance();
        auto* player = playerMgr.GetPlayerBySteamID(mute.target_steam_id);
        if (player)
        {
            player->SetMuted(true);
        }

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool PunishmentManager::IssueGag(const database::Gag& gag)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    try
    {
        // TODO: Save to database via GagRepository

        // Add to cache
        m_gaggedPlayers.insert(gag.target_steam_id);

        // Update player if online
        auto& playerMgr = player::PlayerManager::Instance();
        auto* player = playerMgr.GetPlayerBySteamID(gag.target_steam_id);
        if (player)
        {
            player->SetGagged(true);
        }

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool PunishmentManager::IssueWarning(const database::Warning& warning)
{
    // TODO: Save to database via WarningRepository
    // TODO: Check warning count and auto-punish if threshold reached
    return true;
}

bool PunishmentManager::RemoveBan(int64_t ban_id, int64_t removed_by, const std::string& reason)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    try
    {
        database::BanRepository repo;
        if (!repo.Remove(ban_id, removed_by, reason))
            return false;

        // Remove from cache (find by ban_id)
        for (auto it = m_activeBans.begin(); it != m_activeBans.end(); ++it)
        {
            if (it->second.id == ban_id)
            {
                m_activeBans.erase(it);
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

bool PunishmentManager::RemoveMute(int64_t mute_id, int64_t removed_by, const std::string& reason)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // TODO: Remove from database via MuteRepository
    // TODO: Remove from cache (need to track mute ID -> SteamID mapping)
    return true;
}

bool PunishmentManager::RemoveGag(int64_t gag_id, int64_t removed_by, const std::string& reason)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // TODO: Remove from database via GagRepository
    // TODO: Remove from cache
    return true;
}

void PunishmentManager::ExpireOldPunishments()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Expire bans
    database::BanRepository banRepo;
    banRepo.ExpireOldBans();

    // Remove expired from cache
    for (auto it = m_activeBans.begin(); it != m_activeBans.end();)
    {
        if (it->second.IsExpired())
        {
            it = m_activeBans.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // TODO: Expire mutes and gags
}

PunishmentManager& PunishmentManager::Instance()
{
    static PunishmentManager instance;
    return instance;
}

}  // namespace punishments
