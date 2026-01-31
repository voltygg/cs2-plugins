#include "admin_manager.h"
#include "../database/repositories/admin_repository.h"
#include <algorithm>

namespace admin {

bool AdminManager::LoadAdmins()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    try
    {
        database::AdminRepository repo;
        auto admins = repo.FindAll();

        m_admins.clear();
        for (const auto& admin : admins)
        {
            m_admins[admin.steam_id] = admin;
        }

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool AdminManager::LoadGroups()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    try
    {
        database::AdminGroupRepository repo;
        auto groups = repo.FindAll();

        m_groups.clear();
        for (const auto& group : groups)
        {
            m_groups[group.name] = group;
        }

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool AdminManager::Reload()
{
    bool admins_ok = LoadAdmins();
    bool groups_ok = LoadGroups();
    return admins_ok && groups_ok;
}

bool AdminManager::IsAdmin(int64_t steam_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_admins.find(steam_id) != m_admins.end();
}

const database::Admin* AdminManager::GetAdmin(int64_t steam_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_admins.find(steam_id);
    if (it != m_admins.end())
    {
        return &it->second;
    }

    return nullptr;
}

bool AdminManager::HasPermission(int64_t steam_id, char flag)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_admins.find(steam_id);
    if (it == m_admins.end())
    {
        return false;
    }

    const auto& admin = it->second;

    // Check admin's own flags
    if (admin.HasFlag(flag))
    {
        return true;
    }

    // Check group flags
    std::string effective_flags = ResolveFlags(admin);
    return effective_flags.find(flag) != std::string::npos || effective_flags.find('z') != std::string::npos;
}

bool AdminManager::HasAllPermissions(int64_t steam_id, const std::string& flags)
{
    for (char flag : flags)
    {
        if (!HasPermission(steam_id, flag))
        {
            return false;
        }
    }

    return true;
}

bool AdminManager::HasAnyPermission(int64_t steam_id, const std::string& flags)
{
    for (char flag : flags)
    {
        if (HasPermission(steam_id, flag))
        {
            return true;
        }
    }

    return false;
}

int AdminManager::GetImmunity(int64_t steam_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_admins.find(steam_id);
    if (it == m_admins.end())
    {
        return 0;
    }

    return ResolveImmunity(it->second);
}

bool AdminManager::CanTarget(int64_t admin_steam_id, int64_t target_steam_id)
{
    // Console (steam_id 0) can always target
    if (admin_steam_id == 0)
    {
        return true;
    }

    int admin_immunity = GetImmunity(admin_steam_id);
    int target_immunity = GetImmunity(target_steam_id);

    // Admin can target if their immunity is higher
    return admin_immunity > target_immunity;
}

void AdminManager::AddAdmin(const database::Admin& admin)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_admins[admin.steam_id] = admin;
}

void AdminManager::RemoveAdmin(int64_t steam_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_admins.erase(steam_id);
}

std::string AdminManager::ResolveFlags(const database::Admin& admin)
{
    std::string combined_flags = admin.flags;

    // Add flags from groups
    for (const auto& group_name : admin.groups)
    {
        auto group_it = m_groups.find(group_name);
        if (group_it != m_groups.end())
        {
            const auto& group = group_it->second;

            // Add group's flags
            for (char flag : group.flags)
            {
                if (combined_flags.find(flag) == std::string::npos)
                {
                    combined_flags += flag;
                }
            }

            // TODO: Recursively resolve inherited groups
        }
    }

    return combined_flags;
}

int AdminManager::ResolveImmunity(const database::Admin& admin)
{
    int max_immunity = admin.immunity;

    // Get highest immunity from groups
    for (const auto& group_name : admin.groups)
    {
        auto group_it = m_groups.find(group_name);
        if (group_it != m_groups.end())
        {
            max_immunity = std::max(max_immunity, group_it->second.immunity);

            // TODO: Recursively resolve inherited groups
        }
    }

    return max_immunity;
}

AdminManager& AdminManager::Instance()
{
    static AdminManager instance;
    return instance;
}

} // namespace admin
