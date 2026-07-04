#pragma once

#include "../Entities/Admin.hpp"
#include "../Entities/AdminGroup.hpp"

#include <cstdint>
#include <functional>
#include <pqxx/pqxx>
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

/**
 * Repository for the admins table. The full-table load blocks (load-time / !admin_reload);
 * the chat-style/language writes are fire-and-forget; the freeze writes block because a
 * freeze must be confirmed persisted network-wide before the caller reports success.
 */
class AdminRepository
{
public:
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

    /** All currently frozen admins; the cheap periodic poll behind cross-server propagation.
     *  @p onDone runs on the game thread and is NOT called on DB failure, so callers keep
     *  their cached frozen set instead of accidentally unfreezing everyone. */
    void FindFrozenAsync(std::function<void(std::vector<FrozenAdmin>)> onDone);

private:
    Admin ParseRow(const pqxx::row& row);
};

/** Repository for the admin_groups table. Load-time only. */
class AdminGroupRepository
{
public:
    std::vector<AdminGroup> FindAll();

private:
    AdminGroup ParseRow(const pqxx::row& row);
};

}  // namespace AdminSystem::Database
