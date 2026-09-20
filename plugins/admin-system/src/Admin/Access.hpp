#pragma once

#include "Admin/AdminManager.hpp"
#include "Admin/FreezeManager.hpp"

#include <cstdint>
#include <string_view>

namespace AdminSystem::Admin
{

/**
 * Combines granted permissions, freeze state, and immunity. Commands, menus, and
 * actions use this gate so frozen admins
 * cannot bypass the restriction.
 */
class Access
{
public:
    Access(AdminManager& admins, const FreezeManager& freeze) : _admins(admins), _freeze(freeze) {}

    bool HasPermission(int64_t steamId, std::string_view permission)
    {
        return !_freeze.IsFrozen(steamId) && _admins.HasPermission(steamId, permission);
    }

    /** Punishments only; an admin never outranks themselves, so self-targeting always passes. */
    bool CanPunish(int64_t adminSteamId, int64_t targetSteamId)
    {
        return adminSteamId == targetSteamId || _admins.CanPunish(adminSteamId, targetSteamId);
    }

private:
    AdminManager& _admins;
    const FreezeManager& _freeze;
};

}  // namespace AdminSystem::Admin
