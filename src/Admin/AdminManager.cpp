#include "AdminManager.hpp"

#include "../Database/Repositories/AdminRepository.hpp"

#include <algorithm>

using namespace AdminSystem::Database;

namespace AdminSystem::Admin
{

bool AdminManager::LoadAdmins()
{
    try
    {
        AdminRepository repo;
        auto admins = repo.FindAll();

        _admins.clear();
        _resolvedFlags.clear();

        for (const auto& admin : admins)
        {
            _admins[admin.SteamId] = admin;
        }

        // Resolve and cache flag bitmasks for all loaded admins
        for (auto& [steamId, admin] : _admins)
        {
            _resolvedFlags[steamId] = ResolveFlags(admin);
        }

        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool AdminManager::LoadGroups()
{
    try
    {
        AdminGroupRepository repo;
        auto groups = repo.FindAll();

        _groups.clear();
        for (const auto& group : groups)
        {
            _groups[group.Name] = group;
        }

        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool AdminManager::Reload()
{
    bool groupsOk = LoadGroups();
    bool adminsOk = LoadAdmins();
    return adminsOk && groupsOk;
}

bool AdminManager::IsAdmin(int64_t steamId)
{
    return _admins.find(steamId) != _admins.end();
}

const Database::Admin* AdminManager::GetAdmin(int64_t steamId)
{
    auto it = _admins.find(steamId);
    if (it != _admins.end())
        return &it->second;

    return nullptr;
}

bool AdminManager::HasPermission(int64_t steamId, char flag)
{
    auto it = _resolvedFlags.find(steamId);
    if (it == _resolvedFlags.end())
        return false;

    uint32_t resolved = it->second;

    // Root flag ('z') grants all permissions
    if ((resolved & FlagToBit('z')) != 0)
        return true;

    return (resolved & FlagToBit(flag)) != 0;
}

bool AdminManager::HasAllPermissions(int64_t steamId, const std::string& flags)
{
    auto it = _resolvedFlags.find(steamId);
    if (it == _resolvedFlags.end())
        return false;

    uint32_t resolved = it->second;

    // Root flag grants all permissions
    if ((resolved & FlagToBit('z')) != 0)
        return true;

    for (char flag : flags)
    {
        if ((resolved & FlagToBit(flag)) == 0)
            return false;
    }

    return true;
}

bool AdminManager::HasAnyPermission(int64_t steamId, const std::string& flags)
{
    auto it = _resolvedFlags.find(steamId);
    if (it == _resolvedFlags.end())
        return false;

    uint32_t resolved = it->second;

    // Root flag grants all permissions
    if ((resolved & FlagToBit('z')) != 0)
        return true;

    for (char flag : flags)
    {
        if ((resolved & FlagToBit(flag)) != 0)
            return true;
    }

    return false;
}

int AdminManager::GetImmunity(int64_t steamId)
{
    auto it = _admins.find(steamId);
    if (it == _admins.end())
        return 0;

    return ResolveImmunity(it->second);
}

bool AdminManager::CanTarget(int64_t adminSteamId, int64_t targetSteamId)
{
    // Console (steamId 0) can always target
    if (adminSteamId == 0)
        return true;

    int adminImmunity = GetImmunity(adminSteamId);
    int targetImmunity = GetImmunity(targetSteamId);

    // Admin can target if their immunity is higher
    return adminImmunity > targetImmunity;
}

void AdminManager::AddAdmin(const Database::Admin& admin)
{
    _admins[admin.SteamId] = admin;
    _resolvedFlags[admin.SteamId] = ResolveFlags(admin);
}

void AdminManager::AddGroup(const Database::AdminGroup& group)
{
    _groups[group.Name] = group;
}

void AdminManager::RemoveAdmin(int64_t steamId)
{
    _admins.erase(steamId);
    _resolvedFlags.erase(steamId);
}

uint32_t AdminManager::ResolveFlags(const Database::Admin& admin)
{
    uint32_t bits = 0;

    // Add admin's own flags
    for (char flag : admin.Flags)
    {
        bits |= FlagToBit(flag);
    }

    // Add flags from groups
    for (const auto& groupName : admin.Groups)
    {
        auto groupIt = _groups.find(groupName);
        if (groupIt != _groups.end())
        {
            const auto& group = groupIt->second;
            for (char flag : group.Flags)
            {
                bits |= FlagToBit(flag);
            }

            // TODO: Recursively resolve inherited groups
        }
    }

    return bits;
}

int AdminManager::ResolveImmunity(const Database::Admin& admin)
{
    int maxImmunity = admin.Immunity;

    // Get highest immunity from groups
    for (const auto& groupName : admin.Groups)
    {
        auto groupIt = _groups.find(groupName);
        if (groupIt != _groups.end())
        {
            maxImmunity = std::max(maxImmunity, groupIt->second.Immunity);

            // TODO: Recursively resolve inherited groups
        }
    }

    return maxImmunity;
}

}  // namespace AdminSystem::Admin
