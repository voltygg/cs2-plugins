#pragma once

#include "../Core/Singleton.hpp"
#include "../Database/Entities/Admin.hpp"
#include "../Database/Entities/AdminGroup.hpp"
#include <unordered_map>
#include <string>

namespace AdminSystem::Admin {

/**
 * Central authority for admin permissions, flag resolution, and immunity checks.
 * Admins are loaded from JSON config and/or database. Flags are resolved into
 * uint32_t bitmasks for O(1) permission checks ('a'=bit0 ... 'z'=bit25).
 */
class AdminManager : public Core::Singleton<AdminManager> {
    friend class Core::Singleton<AdminManager>;

public:
    bool LoadAdmins();
    bool LoadGroups();
    bool Reload();
    bool IsAdmin(int64_t steamId);
    const Database::Admin* GetAdmin(int64_t steamId);
    bool HasPermission(int64_t steamId, char flag);
    bool HasAllPermissions(int64_t steamId, const std::string& flags);
    bool HasAnyPermission(int64_t steamId, const std::string& flags);
    int GetImmunity(int64_t steamId);
    bool CanTarget(int64_t adminSteamId, int64_t targetSteamId);
    void AddAdmin(const Database::Admin& admin);
    void AddGroup(const Database::AdminGroup& group);
    void RemoveAdmin(int64_t steamId);

    /** Convert a single flag character ('a'-'z') to a bitmask bit. */
    static uint32_t FlagToBit(char flag)
    {
        if (flag >= 'a' && flag <= 'z')
            return 1u << (flag - 'a');
        return 0;
    }

private:
    AdminManager() = default;

    uint32_t ResolveFlags(const Database::Admin& admin);
    int ResolveImmunity(const Database::Admin& admin);

    std::unordered_map<int64_t, Database::Admin> _admins;
    std::unordered_map<std::string, Database::AdminGroup> _groups;

    /** Cached resolved flag bitmasks per admin steam ID. */
    std::unordered_map<int64_t, uint32_t> _resolvedFlags;
};

} // namespace AdminSystem::Admin
