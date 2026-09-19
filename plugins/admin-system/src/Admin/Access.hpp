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

    /** Immunity only; permission checks apply freeze state. An admin never outranks themselves,
     *  so ask this about somebody else - self-targeting is `Policy::Authorize`'s call. */
    bool CanTarget(int64_t adminSteamId, int64_t targetSteamId)
    {
        return _admins.CanTarget(adminSteamId, targetSteamId);
    }

private:
    AdminManager& _admins;
    const FreezeManager& _freeze;
};

}  // namespace AdminSystem::Admin
