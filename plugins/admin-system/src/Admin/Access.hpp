#pragma once

#include "../Core/Permissions.hpp"
#include "AdminManager.hpp"
#include "FreezeManager.hpp"

#include <cstdint>
#include <string>

namespace AdminSystem::Admin
{

/**
 * What an admin may actually do right now: their granted flags, gated by abuse-protection
 * freezes and immunity.
 *
 * This is the one gate. AdminManager answers "does this admin hold flag x" and FreezeManager
 * answers "is this admin suspended"; every command, menu and action check goes through the
 * composition here, so a frozen admin is denied everything with no surface able to bypass it.
 * Composing the two rather than having them call each other is also what keeps the object
 * graph acyclic.
 */
class Access
{
public:
    Access(AdminManager& admins, const FreezeManager& freeze) : _admins(admins), _freeze(freeze) {}

    bool HasPermission(int64_t steamId, Permission flag)
    {
        return !_freeze.IsFrozen(steamId) && _admins.HasPermission(steamId, static_cast<char>(flag));
    }

    bool HasAnyPermission(int64_t steamId, const std::string& flags)
    {
        return !_freeze.IsFrozen(steamId) && _admins.HasAnyPermission(steamId, flags);
    }

    /** Immunity only - freezing denies the action through the flag check, not the ranking. */
    bool CanTarget(int64_t adminSteamId, int64_t targetSteamId)
    {
        return _admins.CanTarget(adminSteamId, targetSteamId);
    }

    /** True if the admin both holds @p flag and outranks the target. */
    bool CanActOn(int64_t adminSteamId, int64_t targetSteamId, Permission flag)
    {
        return HasPermission(adminSteamId, flag) && CanTarget(adminSteamId, targetSteamId);
    }

private:
    AdminManager& _admins;
    const FreezeManager& _freeze;
};

}  // namespace AdminSystem::Admin
