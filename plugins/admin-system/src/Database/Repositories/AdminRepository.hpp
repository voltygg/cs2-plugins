#pragma once

#include "../Entities/Admin.hpp"
#include "../Entities/AdminGroup.hpp"

#include <CS2Kit/Database/DbResult.hpp>
#include <optional>
#include <pqxx/pqxx>
#include <vector>

namespace AdminSystem::Database
{

/** One frozen admins-table row, as returned by AdminRepository::FindFrozen. */
struct FrozenAdmin
{
    int64_t SteamId = 0;
    std::string Name;
    int64_t FrozenAt = 0;
    int64_t FrozenBy = 0;  // 0 = automatic (rate-limit) freeze
    std::string Reason;
};

/** Repository for CRUD operations on the admins table. */
class AdminRepository
{
public:
    std::optional<Admin> FindBySteamId(int64_t steamId);
    std::vector<Admin> FindAll();
    bool Delete(int64_t steamId);

    /** Persist the per-admin chat overrides set via the admin chat-settings menu. */
    bool UpdateChatStyle(int64_t steamId, bool displayPrefix, const std::string& nameColor,
                         const std::string& messageColor);

    /** Persist the per-admin panel language set via the admin chat-settings menu. */
    bool UpdateLanguage(int64_t steamId, const std::string& lang);

    /** Freeze all of an admin's privileges network-wide. frozenBy 0 = automatic. */
    bool SetFrozen(int64_t steamId, int64_t frozenBy, const std::string& reason);

    /** Lift a freeze. Returns true even if the admin wasn't frozen (idempotent). */
    bool ClearFrozen(int64_t steamId);

    /** All currently frozen admins; the cheap periodic poll behind cross-server propagation.
     *  Returns an error (not an empty list) on DB failure so callers can keep their cached
     *  frozen set instead of accidentally unfreezing everyone. */
    CS2Kit::Database::DbResult<std::vector<FrozenAdmin>> FindFrozen();

private:
    Admin ParseRow(const pqxx::row& row);
};

/** Repository for CRUD operations on the admin_groups table. */
class AdminGroupRepository
{
public:
    std::optional<AdminGroup> FindByName(const std::string& name);
    std::vector<AdminGroup> FindAll();
    bool Delete(const std::string& name);

private:
    AdminGroup ParseRow(const pqxx::row& row);
};

}  // namespace AdminSystem::Database
