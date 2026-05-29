#pragma once

#include "../Core/Permissions.hpp"
#include "../Database/Entities/Admin.hpp"
#include "../Database/Entities/AdminGroup.hpp"

#include <string>
#include <unordered_map>

namespace AdminSystem::Admin
{

using namespace CS2Kit::Core;

/**
 * Resolved chat styling for a single admin, derived from their highest-immunity group
 * that has a non-empty ChatPrefix (with config-driven fallback when none does).
 */
struct AdminChatStyle
{
    std::string Prefix;       /**< E.g., "[ADMIN]". Empty for non-admins. */
    std::string PrefixColor;  /**< Color name; resolved via ChatColors::ParseNamed. */
    std::string NameColor;    /**< Color name for the speaker's display name. */
    std::string MessageColor; /**< Color name for the message body. */
    bool DisplayPrefix = true; /**< Per-admin toggle; suppresses the prefix when false. */

    bool HasPrefix() const { return DisplayPrefix && !Prefix.empty(); }
};

/**
 * Central authority for admin permissions, flag resolution, and immunity checks.
 * Admins are loaded from JSON config and/or database. Flags are resolved into
 * uint32_t bitmasks for O(1) permission checks ('a'=bit0 ... 'z'=bit25).
 */
class AdminManager
{
public:
    AdminManager() = default;

    bool LoadAdmins();
    bool LoadGroups();
    bool Reload();
    bool IsAdmin(int64_t steamId);
    const Database::Admin* GetAdmin(int64_t steamId);
    bool HasPermission(int64_t steamId, char flag);
    bool HasPermission(int64_t steamId, Permission flag) { return HasPermission(steamId, static_cast<char>(flag)); }
    bool HasAllPermissions(int64_t steamId, const std::string& flags);
    bool HasAnyPermission(int64_t steamId, const std::string& flags);
    int GetImmunity(int64_t steamId);
    /**
     * Check if an admin can target a specific player based on their immunity levels.
     */
    bool CanTarget(int64_t adminSteamId, int64_t targetSteamId);

    /** True if the admin both holds @p flag and outranks the target (HasPermission && CanTarget). */
    bool CanExecuteOn(int64_t adminSteamId, int64_t targetSteamId, Permission flag)
    {
        return HasPermission(adminSteamId, flag) && CanTarget(adminSteamId, targetSteamId);
    }
    void AddAdmin(const Database::Admin& admin);
    void AddGroup(const Database::AdminGroup& group);
    void RemoveAdmin(int64_t steamId);

    /**
     * Resolve the chat styling that should decorate this admin's chat lines. Returns an
     * empty `AdminChatStyle` (HasPrefix() == false) for non-admins.
     */
    AdminChatStyle GetChatStyle(int64_t steamId);

    /**
     * Persist + apply per-admin chat-style overrides. Empty color strings revert that slot
     * back to the admin's group default. Returns false if the admin is unknown or the DB
     * write failed; the in-memory cache is only updated on a successful write.
     */
    bool UpdateChatStyle(int64_t steamId, bool displayPrefix, const std::string& nameColor,
                         const std::string& messageColor);

    /**
     * Persist + apply an admin's panel language. Returns false if the admin is unknown or the
     * DB write failed; the in-memory row is only updated on a successful write.
     */
    bool UpdateLanguage(int64_t steamId, const std::string& lang);

    /** Convert a single flag character ('a'-'z') to a bitmask bit. */
    static uint32_t FlagToBit(char flag)
    {
        if (flag >= 'a' && flag <= 'z')
        {
            return 1u << (flag - 'a');
        }
        return 0;
    }

private:
    uint32_t ResolveFlags(const Database::Admin& admin);
    int ResolveImmunity(const Database::Admin& admin);

    std::unordered_map<int64_t, Database::Admin> _admins;
    std::unordered_map<std::string, Database::AdminGroup> _groups;

    /** Cached resolved flag bitmasks per admin steam ID. */
    std::unordered_map<int64_t, uint32_t> _resolvedFlags;

    /** Cached resolved chat styles per admin steam ID; invalidated on Reload(). */
    std::unordered_map<int64_t, AdminChatStyle> _resolvedStyles;
};

}  // namespace AdminSystem::Admin
