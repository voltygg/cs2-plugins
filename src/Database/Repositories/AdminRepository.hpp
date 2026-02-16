#pragma once

#include "../Entities/Admin.hpp"
#include "../Entities/AdminGroup.hpp"
#include <pqxx/pqxx>
#include <optional>
#include <vector>

namespace AdminSystem::Database {

class AdminRepository {
public:
    std::optional<Admin> FindBySteamId(int64_t steamId);
    std::vector<Admin> FindAll();
    bool Delete(int64_t steamId);

private:
    Admin ParseRow(const pqxx::row& row);
};

class AdminGroupRepository {
public:
    std::optional<AdminGroup> FindByName(const std::string& name);
    std::vector<AdminGroup> FindAll();
    bool Delete(const std::string& name);

private:
    AdminGroup ParseRow(const pqxx::row& row);
};

} // namespace AdminSystem::Database
