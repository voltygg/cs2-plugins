#include "AdminManager.hpp"

#include "../Core/Config.hpp"
#include "../Core/Managers.hpp"
#include "../Database/Repositories/AdminRepository.hpp"
#include "../Database/Repositories/ServerRepository.hpp"

#include <CS2Kit/Utils/Log.hpp>
#include <algorithm>
#include <utility>

namespace AdminSystem::Admin
{

namespace Db = AdminSystem::Database;
namespace Log = CS2Kit::Utils::Log;
using Core::ConfigManager;
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

        // Merge this server's admin_server_groups grants into the in-memory Groups vectors so
        // flag/immunity/chat-style resolution below sees the effective per-server set. Grants
        // for unknown admins are skipped, like unknown group names in admins.groups.
        Db::AdminServerGroupRepository serverGroupRepo;
        for (auto& [steamId, groupNames] : serverGroupRepo.FindByServerTag(App().Config.GetServer().tag))
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
        {
            _resolvedFlags[steamId] = ResolveFlags(admin);
        }

        Log::Info("Loaded {} admin(s) from database.", _admins.size());
        return true;
    }
    catch (const std::exception& ex)
    {
        Log::Error("LoadAdmins failed: {}", ex.what());
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
        _resolvedStyles.clear();
        for (const auto& group : groups)
        {
            _groups[group.Name] = group;
        }

        Log::Info("Loaded {} admin group(s) from database.", _groups.size());
        return true;
    }
    catch (const std::exception& ex)
    {
        Log::Error("LoadGroups failed: {}", ex.what());
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
    // Abuse-protection: a frozen admin is denied everything. Every permission surface
    // (commands, menu, actions) funnels through these three methods, so this is the one gate.
    if (App().Freeze.IsFrozen(steamId))
        return false;

    auto it = _resolvedFlags.find(steamId);
    return it != _resolvedFlags.end() && HasBit(it->second, flag);
}

bool AdminManager::HasAllPermissions(int64_t steamId, const std::string& flags)
{
    if (App().Freeze.IsFrozen(steamId))
        return false;

    auto it = _resolvedFlags.find(steamId);
    if (it == _resolvedFlags.end())
        return false;

    for (char flag : flags)
    {
        if (!HasBit(it->second, flag))
            return false;
    }

    return true;
}

bool AdminManager::HasAnyPermission(int64_t steamId, const std::string& flags)
{
    if (App().Freeze.IsFrozen(steamId))
        return false;

    auto it = _resolvedFlags.find(steamId);
    if (it == _resolvedFlags.end())
        return false;

    for (char flag : flags)
    {
        if (HasBit(it->second, flag))
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

    // Self-targeting always allowed - admins can apply fun effects to themselves,
    // and self-punishments (voice-mute/text-mute/kick) are harmless / occasionally useful for testing.
    if (adminSteamId == targetSteamId)
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
        const auto& fallback = App().Config.GetChat();
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

bool AdminManager::UpdateChatStyle(int64_t steamId, bool displayPrefix, const std::string& nameColor,
                                   const std::string& messageColor)
{
    auto it = _admins.find(steamId);
    if (it == _admins.end())
        return false;

    // Cache-first: the next chat line uses the new style immediately; the persist rides the worker.
    Database::AdminRepository{}.UpdateChatStyle(steamId, displayPrefix, nameColor, messageColor);

    auto& admin = it->second;
    admin.DisplayPrefix = displayPrefix;
    admin.NameColor = nameColor;
    admin.MessageColor = messageColor;

    _resolvedStyles.erase(steamId);
    return true;
}

bool AdminManager::UpdateLanguage(int64_t steamId, const std::string& lang)
{
    auto it = _admins.find(steamId);
    if (it == _admins.end())
        return false;

    Database::AdminRepository{}.UpdateLanguage(steamId, lang);
    it->second.Language = lang;
    return true;
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
