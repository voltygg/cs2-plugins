#include "Admin/AdminManager.hpp"

#include "Config/ConfigManager.hpp"
#include "Core/Permissions.hpp"
#include "Database/Repositories/Admins.hpp"

#include <VoltMod/Core/Log.hpp>
#include <algorithm>
#include <format>
#include <utility>

namespace AdminSystem::Admin
{

namespace Db = AdminSystem::Database;
namespace Log = VoltMod::Log;
using Db::AdminRepository;

bool AdminManager::LoadAdmins()
{
    auto admins = _repos.Admins.FindAll();

    _admins.clear();
    _resolvedPermissions.clear();
    _resolvedStyles.clear();

    for (const auto& admin : admins)
        _admins[admin.SteamId] = admin;

    // Merge server grants into each admin's effective group list.
    for (auto& [steamId, groupNames] : _repos.Admins.FindGroupsForServer(_config.Get().server.tag))
    {
        auto it = _admins.find(steamId);
        if (it == _admins.end())
            continue;
        auto& groups = it->second.Groups;
        for (auto& name : groupNames)
        {
            if (std::find(groups.begin(), groups.end(), name) == groups.end())
                groups.push_back(std::move(name));
        }
    }

    for (auto& [steamId, admin] : _admins)
        _resolvedPermissions[steamId] = ResolvePermissions(admin);

    Log::Info("Loaded {} admin(s) from database.", _admins.size());
    return true;
}

bool AdminManager::LoadGroups()
{
    auto groups = _repos.Admins.FindAllGroups();

    _groups.clear();
    _resolvedStyles.clear();
    for (const auto& group : groups)
        _groups[group.Name] = group;

    Log::Info("Loaded {} admin group(s) from database.", _groups.size());
    return true;
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

bool AdminManager::HasPermission(int64_t steamId, std::string_view permission)
{
    auto it = _resolvedPermissions.find(steamId);
    if (it == _resolvedPermissions.end())
        return false;

    const PermissionSet& granted = it->second;
    if (granted.contains(Permission::Root) || granted.contains(permission))
        return true;

    for (size_t dot = permission.find('.'); dot != std::string_view::npos; dot = permission.find('.', dot + 1))
    {
        if (granted.contains(std::format("{}*", permission.substr(0, dot + 1))))
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

bool AdminManager::CanPunish(int64_t adminSteamId, int64_t targetSteamId)
{
    return GetImmunity(adminSteamId) > GetImmunity(targetSteamId);
}

void AdminManager::AddAdmin(const Database::Admin& admin)
{
    _admins[admin.SteamId] = admin;
    _resolvedPermissions[admin.SteamId] = ResolvePermissions(admin);
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
    _resolvedPermissions.erase(steamId);
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
        const auto& fallback = _config.Get().chat;
        style.Prefix = fallback.fallbackPrefix;
        style.PrefixColor = fallback.fallbackPrefixColor;
        style.NameColor = fallback.fallbackNameColor;
        style.MessageColor = fallback.fallbackMessageColor;
    }

    // Per-admin overrides win over group/fallback. Empty strings keep the inherited value
    // so admins can override individual slots without losing the rest of their group's styling.
    const auto& adminRow = adminIt->second;
    if (!adminRow.NameColor.empty())
        style.NameColor = adminRow.NameColor;
    if (!adminRow.MessageColor.empty())
        style.MessageColor = adminRow.MessageColor;
    style.DisplayPrefix = adminRow.DisplayPrefix;

    _resolvedStyles[steamId] = style;
    return style;
}

void AdminManager::UpdateChatStyleAsync(int64_t steamId, bool displayPrefix, const std::string& nameColor,
                                        const std::string& messageColor)
{
    auto it = _admins.find(steamId);
    if (it == _admins.end())
        return;

    // Cache-first: the next chat line uses the new style immediately; the persist rides the worker.
    _repos.Admins.UpdateChatStyleAsync(steamId, displayPrefix, nameColor, messageColor);

    auto& admin = it->second;
    admin.DisplayPrefix = displayPrefix;
    admin.NameColor = nameColor;
    admin.MessageColor = messageColor;

    _resolvedStyles.erase(steamId);
}

AdminManager::PermissionSet AdminManager::ResolvePermissions(const Database::Admin& admin)
{
    PermissionSet granted(admin.Permissions.begin(), admin.Permissions.end());

    for (const auto& groupName : admin.Groups)
    {
        auto groupIt = _groups.find(groupName);
        if (groupIt != _groups.end())
        {
            granted.insert(groupIt->second.Permissions.begin(), groupIt->second.Permissions.end());

            // TODO: Recursively resolve inherited groups
        }
    }

    return granted;
}

int AdminManager::ResolveImmunity(const Database::Admin& admin)
{
    int maxImmunity = 0;

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
