#include "AdminManager.hpp"

#include "../Core/Config.hpp"
#include "../Database/Repositories/AdminRepository.hpp"

#include <algorithm>

namespace AdminSystem::Admin
{

namespace Db = AdminSystem::Database;
using Db::Admin;
using Db::AdminGroup;
using Db::AdminGroupRepository;
using Db::AdminRepository;

bool AdminManager::LoadAdmins()
{
    try
    {
        AdminRepository repo;
        auto admins = repo.FindAll();

        _admins.clear();
        _resolvedFlags.clear();
        _resolvedStyles.clear();

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
        _resolvedStyles.clear();  // group changes can shift the resolved prefix for any admin
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
    {
        return false;
    }

    uint32_t resolved = it->second;

    // Root flag ('z') grants all permissions
    if ((resolved & FlagToBit('z')) != 0)
    {
        return true;
    }

    return (resolved & FlagToBit(flag)) != 0;
}

bool AdminManager::HasAllPermissions(int64_t steamId, const std::string& flags)
{
    auto it = _resolvedFlags.find(steamId);
    if (it == _resolvedFlags.end())
    {
        return false;
    }

    uint32_t resolved = it->second;

    // Root flag grants all permissions
    if ((resolved & FlagToBit('z')) != 0)
    {
        return true;
    }

    for (char flag : flags)
    {
        if ((resolved & FlagToBit(flag)) == 0)
        {
            return false;
        }
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
    _resolvedStyles.erase(admin.SteamId);
}

void AdminManager::AddGroup(const Database::AdminGroup& group)
{
    _groups[group.Name] = group;
    _resolvedStyles.clear();  // any admin in this group may now resolve to a different prefix
}

void AdminManager::RemoveAdmin(int64_t steamId)
{
    _admins.erase(steamId);
    _resolvedFlags.erase(steamId);
    _resolvedStyles.erase(steamId);
}

AdminChatStyle AdminManager::GetChatStyle(int64_t steamId)
{
    if (auto it = _resolvedStyles.find(steamId); it != _resolvedStyles.end())
    {
        return it->second;
    }

    AdminChatStyle style;

    auto adminIt = _admins.find(steamId);
    if (adminIt == _admins.end())
    {
        return style;  // Non-admin: empty style.
    }

    // Pick the highest-immunity group that has a non-empty ChatPrefix. This way an admin in
    // both "moderator" and "headadmin" gets the headadmin tag, while an admin in only "vip"
    // (no prefix) falls through to the configured default.
    const Database::AdminGroup* chosen = nullptr;
    for (const auto& groupName : adminIt->second.Groups)
    {
        auto groupIt = _groups.find(groupName);
        if (groupIt == _groups.end())
            continue;
        if (groupIt->second.ChatPrefix.empty())
            continue;
        if (!chosen || groupIt->second.Immunity > chosen->Immunity)
            chosen = &groupIt->second;
    }

    if (chosen)
    {
        style.Prefix = chosen->ChatPrefix;
        style.PrefixColor = chosen->PrefixColor;
        style.NameColor = chosen->NameColor;
        style.MessageColor = chosen->MessageColor;
    }
    else
    {
        const auto& fallback = AdminSystem::Core::ConfigManager::Instance().GetChatConfig();
        style.Prefix = fallback.FallbackPrefix;
        style.PrefixColor = fallback.FallbackPrefixColor;
        style.NameColor = fallback.FallbackNameColor;
        style.MessageColor = fallback.FallbackMessageColor;
    }

    _resolvedStyles[steamId] = style;
    return style;
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
