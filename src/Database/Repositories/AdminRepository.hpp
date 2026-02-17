#pragma once

#include "../Entities/Admin.hpp"
#include "../Entities/AdminGroup.hpp"

#include <optional>
#include <pqxx/pqxx>
#include <vector>

namespace AdminSystem::Database
{

/** Repository for CRUD operations on the admins table. */
class AdminRepository
{
public:
    std::optional<Admin> FindBySteamId(int64_t steamId);
    std::vector<Admin> FindAll();
    bool Delete(int64_t steamId);

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
