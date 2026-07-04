#pragma once

#include "../Database/Repositories/AdminRepository.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace AdminSystem::Admin
{

/**
 * Admin-abuse protection. A frozen admin has ALL admin permissions denied - the gate lives
 * inside AdminManager::HasPermission/HasAnyPermission/HasAllPermissions, which every command,
 * menu, and action check funnels through. Freeze state is stored on the shared admins table,
 * so it applies network-wide; RefreshFromDatabase() runs on the 60s timer to pick up freezes
 * issued from other servers. Freeze/unfreeze history is recorded in admin_activity, and this
 * manager's _frozen map is the single in-memory source of freeze truth.
 */
class FreezeManager
{
public:
    FreezeManager() = default;

    /**
     * Re-read the frozen set from the database (boot, 60s timer, !admin_reload) and notify
     * newly-frozen admins who are online here. On a DB error the cached set is kept as-is,
     * so an outage never silently unfreezes anyone.
     */
    void RefreshFromDatabase();

    bool IsFrozen(int64_t steamId) const { return _frozen.contains(steamId); }
    const Database::FrozenAdmin* GetFrozen(int64_t steamId) const;

    /** The live frozen set, keyed by admin steam ID (for the list/unfreeze commands). */
    const std::unordered_map<int64_t, Database::FrozenAdmin>& Frozen() const { return _frozen; }

    /** Manual freeze. The command layer validates admin-ness, self-targeting, and immunity. */
    bool Freeze(int64_t targetSteamId, const std::string& targetName, int64_t bySteamId, const std::string& byName,
                const std::string& reason);

    /** Lift the freeze for @p targetSteamId. Returns false if not frozen or the DB write failed. */
    bool Unfreeze(int64_t targetSteamId, int64_t bySteamId, const std::string& byName);

    /**
     * Entry point for the IssuePunishment choke point: writes the admin_activity audit row,
     * then runs the auto-freeze rate check for the acting admin when abuseProtection is
     * enabled (console and root admins are exempt).
     */
    void RecordPunishment(int64_t adminSteamId, const std::string& adminName, const std::string& action,
                          int64_t targetSteamId, const std::string& targetName, const std::string& detail);

    /** Tell a frozen admin their privileges are suspended, if they are on this server. */
    void NotifyFrozen(int64_t steamId);

private:
    /** Plain admin_activity write, no enforcement attached. */
    void RecordAudit(int64_t adminSteamId, const std::string& adminName, const std::string& action,
                     int64_t targetSteamId, const std::string& targetName, const std::string& detail);

    /** Shared body of manual + automatic freezing: persist, cache, audit, notify. The differing
     *  log line and broadcast stay at the call sites. */
    bool ApplyFreeze(int64_t steamId, const std::string& name, int64_t bySteamId, const std::string& byName,
                     const std::string& reason);

    void CheckAutoFreeze(int64_t adminSteamId, const std::string& adminName);

    std::unordered_map<int64_t, Database::FrozenAdmin> _frozen; /**< keyed by admin steam ID */
};

}  // namespace AdminSystem::Admin
