#pragma once

#include "../Entities/Admin.hpp"
#include "../Entities/AdminGroup.hpp"

#include <VoltMod/Database/Api.hpp>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace AdminSystem::Database
{

/** One frozen admins-table row, as returned by AdminRepository::FindFrozenAsync. */
struct FrozenAdmin
{
    int64_t SteamId = 0;
    std::string Name;
    int64_t FrozenAt = 0;
    int64_t FrozenBy = 0;  // 0 = automatic (rate-limit) freeze
    std::string Reason;
};

/** Repository for admins. Loads and freeze writes block; style and language writes do not. */
class AdminRepository
{
public:
    explicit AdminRepository(VoltMod::Database& db) : _db(db) {}

    std::vector<Admin> FindAll();

    /** Persist the per-admin chat overrides set via the admin chat-settings menu. */
    void UpdateChatStyle(int64_t steamId, bool displayPrefix, const std::string& nameColor,
                         const std::string& messageColor);

    /** Persist the per-admin panel language set via the admin chat-settings menu. */
    void UpdateLanguage(int64_t steamId, const std::string& lang);

    /** Freeze all of an admin's privileges network-wide. frozenBy 0 = automatic. Blocking. */
    bool SetFrozen(int64_t steamId, int64_t frozenBy, const std::string& reason);

    /** Lift a freeze. Returns true even if the admin wasn't frozen (idempotent). Blocking. */
    bool ClearFrozen(int64_t steamId);

    /** Poll frozen admins for cross-server propagation. On DB failure, @p onDone is not called. */
    void FindFrozenAsync(std::function<void(std::vector<FrozenAdmin>)> onDone);

private:
    VoltMod::Database& _db;
};

/** Repository for the admin_groups table. Load-time only. */
class AdminGroupRepository
{
public:
    explicit AdminGroupRepository(VoltMod::Database& db) : _db(db) {}

    std::vector<AdminGroup> FindAll();

private:
    VoltMod::Database& _db;
};

}  // namespace AdminSystem::Database
