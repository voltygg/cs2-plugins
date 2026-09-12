#pragma once

#include "../Config/ConfigManager.hpp"
#include "../Core/ChatService.hpp"
#include "../Database/Repositories.hpp"
#include "PunishType.hpp"

#include <VoltMod/Runtime.hpp>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace AdminSystem::Punishments
{

/** Active bans, mutes and warnings. The cache is updated synchronously so a punishment bites the
 *  frame it is issued; the database write is async and backfills the row id when it returns. */
class PunishmentManager
{
public:
    PunishmentManager(Database::Repositories& repos, const Config::ConfigManager& config, VoltMod::Runtime& runtime,
                      Core::ChatService& chat)
        : _repos(repos), _config(config), _rt(runtime), _chat(chat)
    {}

    bool LoadActivePunishments();

    /** Snapshot of the cached active rows of @p kind, newest first (drives the lift menus). */
    std::vector<Database::Punishment> GetActive(PunishType kind) const;

    /** The active row of @p kind against @p steamId, dropping it when it has since expired. */
    std::optional<Database::Punishment> GetActive(PunishType kind, int64_t steamId);

    /** Whether @p steamId is currently punished with @p kind. The tick-rate check: it does not
     *  sweep expiries, which the minute timer does. */
    bool IsPunished(PunishType kind, int64_t steamId) const;

    /** Persist and apply @p record: cache, broadcast, kick a banned player, escalate a warning
     *  past the threshold. False for a Kick, which stores no row. */
    bool Issue(Database::Punishment& record);

    /** Lift by row id. False when it was already gone, possibly lifted on another server. */
    bool Remove(PunishType kind, int64_t recordId, int64_t removedBy, const std::string& reason);

    /** Lift whatever of @p kind is active against @p steamId. False when nothing was. */
    bool RemoveBySteamId(PunishType kind, int64_t steamId, int64_t removedBy, const std::string& reason);

    /** Mark expired rows inactive in the DB and rebuild the caches (all async). */
    void ExpireOldPunishments();

    /** Kick @p slot next frame, and only if @p steamId still occupies it. Deferred because
     *  callers run inside target hooks, where disconnecting interrupts the virtual call. */
    void KickDeferred(int slot, int64_t steamId, std::string reason);

private:
    using Cache = std::unordered_map<int64_t, Database::Punishment>;  ///< keyed by TargetSteamId

    /** Whether @p kind is cached. A cache holds one row per player, which a warning breaks and a
     *  kick has none of, so those two are counted in the database instead. */
    static bool IsCached(PunishType kind)
    {
        return kind == PunishType::Ban || kind == PunishType::VoiceMute || kind == PunishType::TextMute;
    }

    Cache& CacheFor(PunishType kind) { return _active[static_cast<size_t>(kind)]; }
    const Cache& CacheFor(PunishType kind) const { return _active[static_cast<size_t>(kind)]; }

    /** Re-query every active row off-thread and swap the caches when they arrive. */
    void RefreshCachesAsync();
    void SwapCaches(std::vector<Database::Punishment> rows);
    /** Auto-ban once a player has collected the configured number of active warnings. */
    void EscalateWarning(const Database::Punishment& warning);

    Database::Repositories& _repos;
    const Config::ConfigManager& _config;
    VoltMod::Runtime& _rt;
    Core::ChatService& _chat;
    VoltMod::PerSlot<VoltMod::Subscription> _pendingKick;

    /** Indexed by PunishType; the slots @ref IsCached rejects stay empty. */
    std::array<Cache, 5> _active;
};

}  // namespace AdminSystem::Punishments
