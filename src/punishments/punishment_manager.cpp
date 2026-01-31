#include "punishment_manager.h"
#include "../database/repositories/ban_repository.h"
#include "../player/player_manager.h"

using namespace std;
using namespace database;
using namespace player;

namespace punishments {

bool PunishmentManager::LoadActivePunishments()
{
    lock_guard<mutex> lock(m_mutex);

    try
    {
        BanRepository banRepo;
        auto bans = banRepo.FindAllActive();

        m_activeBans.clear();
        for (const auto& ban : bans)
        {
            m_activeBans[ban.target_steam_id] = ban;
        }

        // TODO: Load mutes and gags

        return true;
    }
    catch (const exception& e)
    {
        return false;
    }
}

optional<Ban> PunishmentManager::GetActiveBan(int64_t steam_id)
{
    lock_guard<mutex> lock(m_mutex);

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

    return nullopt;
}

bool PunishmentManager::IsMuted(int64_t steam_id)
{
    lock_guard<mutex> lock(m_mutex);
    return m_mutedPlayers.contains(steam_id);
}

bool PunishmentManager::IsGagged(int64_t steam_id)
{
    lock_guard<mutex> lock(m_mutex);
    return m_gaggedPlayers.contains(steam_id);
}

bool PunishmentManager::IssueBan(const Ban& ban)
{
    lock_guard<mutex> lock(m_mutex);

    try
    {
        // Save to database
        BanRepository repo;
        if (!repo.Create(ban))
            return false;

        // Add to cache
        m_activeBans[ban.target_steam_id] = ban;

        // Kick player if online
        auto& playerMgr = PlayerManager::Instance();
        auto* player = playerMgr.GetPlayerBySteamID(ban.target_steam_id);
        if (player)
        {
            // TODO: Kick player from server
        }

        return true;
    }
    catch (const exception& e)
    {
        return false;
    }
}

bool PunishmentManager::IssueMute(const Mute& mute)
{
    lock_guard<mutex> lock(m_mutex);

    try
    {
        // TODO: Save to database via MuteRepository

        // Add to cache
        m_mutedPlayers.insert(mute.target_steam_id);

        // Update player if online
        auto& playerMgr = PlayerManager::Instance();
        auto* player = playerMgr.GetPlayerBySteamID(mute.target_steam_id);
        if (player)
        {
            player->SetMuted(true);
        }

        return true;
    }
    catch (const exception& e)
    {
        return false;
    }
}

bool PunishmentManager::IssueGag(const Gag& gag)
{
    lock_guard<mutex> lock(m_mutex);

    try
    {
        // TODO: Save to database via GagRepository

        // Add to cache
        m_gaggedPlayers.insert(gag.target_steam_id);

        // Update player if online
        auto& playerMgr = PlayerManager::Instance();
        auto* player = playerMgr.GetPlayerBySteamID(gag.target_steam_id);
        if (player)
        {
            player->SetGagged(true);
        }

        return true;
    }
    catch (const exception& e)
    {
        return false;
    }
}

bool PunishmentManager::IssueWarning(const Warning& warning)
{
    try
    {
        // TODO: Save to database via WarningRepository

        // TODO: Check warning count and auto-punish if threshold reached

        return true;
    }
    catch (const exception& e)
    {
        return false;
    }
}

bool PunishmentManager::RemoveBan(int64_t ban_id, int64_t removed_by, const string& reason)
{
    lock_guard<mutex> lock(m_mutex);

    try
    {
        BanRepository repo;
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
    catch (const exception& e)
    {
        return false;
    }
}

bool PunishmentManager::RemoveMute(int64_t mute_id, int64_t removed_by, const string& reason)
{
    lock_guard<mutex> lock(m_mutex);

    try
    {
        // TODO: Remove from database via MuteRepository

        // TODO: Remove from cache (need to track mute ID -> SteamID mapping)

        return true;
    }
    catch (const exception& e)
    {
        return false;
    }
}

bool PunishmentManager::RemoveGag(int64_t gag_id, int64_t removed_by, const string& reason)
{
    lock_guard<mutex> lock(m_mutex);

    try
    {
        // TODO: Remove from database via GagRepository

        // TODO: Remove from cache

        return true;
    }
    catch (const exception& e)
    {
        return false;
    }
}

void PunishmentManager::ExpireOldPunishments()
{
    lock_guard<mutex> lock(m_mutex);

    // Expire bans
    BanRepository banRepo;
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

} // namespace punishments
