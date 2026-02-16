#include "AdminRepository.hpp"
#include "../Database.hpp"

namespace AdminSystem::Database {

// AdminRepository implementation

std::optional<Admin> AdminRepository::FindBySteamId(int64_t steamId)
{
    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "find_admin_by_steamid",
            "SELECT * FROM admins WHERE steam_id = $1",
            steamId);

        if (result.empty())
        {
            return std::nullopt;
        }

        return ParseRow(result[0]);
    }
    catch (const std::exception& e)
    {
        return std::nullopt;
    }
}

std::vector<Admin> AdminRepository::FindAll()
{
    std::vector<Admin> admins;

    try
    {
        auto result = Database::Instance().Execute("SELECT * FROM admins");

        for (const auto& row : result)
        {
            admins.push_back(ParseRow(row));
        }
    }
    catch (const std::exception& e)
    {
        // Log error
    }

    return admins;
}

bool AdminRepository::Delete(int64_t steamId)
{
    try
    {
        Database::Instance().ExecutePrepared(
            "delete_admin",
            "DELETE FROM admins WHERE steam_id = $1",
            steamId);

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

Admin AdminRepository::ParseRow(const pqxx::row& row)
{
    Admin admin;
    admin.Id = row["id"].as<int64_t>();
    admin.SteamId = row["steam_id"].as<int64_t>();
    admin.Name = row["name"].c_str();
    admin.Flags = row["flags"].c_str();
    admin.Immunity = row["immunity"].as<int32_t>();
    admin.CreatedAt = row["created_at"].as<int64_t>();
    admin.UpdatedAt = row["updated_at"].as<int64_t>();

    // Parse groups array (PostgreSQL text array)
    // TODO: Implement proper array parsing
    admin.Groups = {};

    admin.BuildFlagBits();
    return admin;
}

// AdminGroupRepository implementation

std::optional<AdminGroup> AdminGroupRepository::FindByName(const std::string& name)
{
    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "find_group_by_name",
            "SELECT * FROM admin_groups WHERE name = $1",
            name);

        if (result.empty())
        {
            return std::nullopt;
        }

        return ParseRow(result[0]);
    }
    catch (const std::exception& e)
    {
        return std::nullopt;
    }
}

std::vector<AdminGroup> AdminGroupRepository::FindAll()
{
    std::vector<AdminGroup> groups;

    try
    {
        auto result = Database::Instance().Execute("SELECT * FROM admin_groups");

        for (const auto& row : result)
        {
            groups.push_back(ParseRow(row));
        }
    }
    catch (const std::exception& e)
    {
        // Log error
    }

    return groups;
}

bool AdminGroupRepository::Delete(const std::string& name)
{
    try
    {
        Database::Instance().ExecutePrepared(
            "delete_group",
            "DELETE FROM admin_groups WHERE name = $1",
            name);

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

AdminGroup AdminGroupRepository::ParseRow(const pqxx::row& row)
{
    AdminGroup group;
    group.Id = row["id"].as<int64_t>();
    group.Name = row["name"].c_str();
    group.Flags = row["flags"].c_str();
    group.Immunity = row["immunity"].as<int32_t>();
    group.CreatedAt = row["created_at"].as<int64_t>();
    group.UpdatedAt = row["updated_at"].as<int64_t>();

    // Parse inherits array (PostgreSQL text array)
    // TODO: Implement proper array parsing
    group.Inherits = {};

    group.BuildFlagBits();
    return group;
}

} // namespace AdminSystem::Database
