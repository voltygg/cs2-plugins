#pragma once

#include "../Entities.hpp"

#include <VoltMod/Database/Api.hpp>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace AdminSystem::Database
{

/** Admins, the groups they inherit from, and the per-server grants on top. The reads block and
 *  are load-time only; every write is fire-and-forget. */
class AdminRepository
{
public:
    explicit AdminRepository(VoltMod::Database& db) : _db(db) {}

    std::vector<Admin> FindAll();
    std::vector<AdminGroup> FindAllGroups();

    /** steamId -> group names granted on @p serverTag, on top of the global `admins.groups`. */
    std::unordered_map<int64_t, std::vector<std::string>> FindGroupsForServer(const std::string& serverTag);

    /** Persist the per-admin chat overrides set via the admin chat-settings menu. */
    void UpdateChatStyleAsync(int64_t steamId, bool displayPrefix, const std::string& nameColor,
                              const std::string& messageColor);

    /** Persist the per-admin panel language set via the admin chat-settings menu. */
    void UpdateLanguageAsync(int64_t steamId, const std::string& lang);

    /** Freeze every privilege this admin holds, network-wide. frozenBy 0 = automatic. */
    void SetFrozenAsync(int64_t steamId, int64_t frozenBy, const std::string& reason);

    /** Lift a freeze. Idempotent: an admin who was not frozen is left alone. */
    void ClearFrozenAsync(int64_t steamId);

    /** Poll frozen admins for cross-server propagation. On DB failure, @p onDone is not called. */
    void FindFrozenAsync(std::function<void(std::vector<FrozenAdmin>)> onDone);

private:
    VoltMod::Database& _db;
};

}  // namespace AdminSystem::Database
