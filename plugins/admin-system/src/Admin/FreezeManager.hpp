#pragma once

#include "../Core/ChatService.hpp"
#include "../Core/Config.hpp"
#include "../Database/Repositories/AdminRepository.hpp"
#include "AdminManager.hpp"

#include <VoltMod/Database/PostgresDatabase.hpp>
#include <VoltMod/Runtime.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace AdminSystem::Admin
{

/**
 * Admin-abuse protection. A frozen admin has ALL admin permissions denied - the gate lives in
 * @ref Access, which every command, menu, and action check funnels through. Freeze state is
 * stored on the shared admins table,
 * so it applies network-wide; RefreshFromDatabase() runs on the 60s timer to pick up freezes
 * issued from other servers. Freeze/unfreeze history is recorded in admin_activity, and this
 * manager's _frozen map is the single in-memory source of freeze truth.
 */
class FreezeManager
{
public:
    FreezeManager(VoltMod::PostgresDatabase& db, const Core::ConfigManager& config, VoltMod::Runtime& runtime,
                  Core::ChatService& chat, AdminManager& admins)
        : _db(db), _config(config), _rt(runtime), _chat(chat), _admins(admins)
    {}

    /**
     * Re-read the frozen set from the database (boot, 60s timer, !admin_reload) and notify
     * newly-frozen admins who are online here. On a DB error the cached set is kept as-is,
     * so an outage never silently unfreezes anyone.
     */
    void RefreshFromDatabase();

    bool IsFrozen(int64_t steamId) const { return _frozen.contains(steamId); }
    /** A copy of the frozen row, or nullopt when @p steamId is not frozen. By value because
     *  Unfreeze() and the periodic refresh both erase from the live map. */
    std::optional<Database::FrozenAdmin> GetFrozen(int64_t steamId) const;

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
    void RecordPunishment(int64_t adminSteamId, std::string_view adminName, std::string_view action,
                          int64_t targetSteamId, std::string_view targetName, std::string_view detail);

    /** Tell a frozen admin their privileges are suspended, if they are on this server. */
    void NotifyFrozen(int64_t steamId);

    /** @ref NotifyFrozen on the next game frame, for the connect hook: a client that has only
     *  just connected does not render a line sent from inside it. Owned per slot, so a reconnect
     *  replaces the pending notice and unload drops it. */
    void NotifyFrozenSoon(int slot, int64_t steamId);

private:
    VoltMod::PostgresDatabase& _db;
    const Core::ConfigManager& _config;
    VoltMod::Runtime& _rt;
    Core::ChatService& _chat;
    AdminManager& _admins;
    VoltMod::PerSlot<VoltMod::Subscription> _pendingNotice;

    /** Plain admin_activity write, no enforcement attached. */
    void RecordAudit(int64_t adminSteamId, std::string_view adminName, std::string_view action, int64_t targetSteamId,
                     std::string_view targetName, std::string_view detail);

    /** Shared body of manual + automatic freezing: persist, cache, audit, notify. The differing
     *  log line and broadcast stay at the call sites. */
    bool ApplyFreeze(int64_t steamId, const std::string& name, int64_t bySteamId, const std::string& byName,
                     const std::string& reason);

    void CheckAutoFreeze(int64_t adminSteamId, std::string_view adminName);

    std::unordered_map<int64_t, Database::FrozenAdmin> _frozen; /**< keyed by admin steam ID */
};

}  // namespace AdminSystem::Admin
