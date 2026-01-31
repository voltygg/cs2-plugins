#include "admin_repository.h"

#include "../../utils/time.h"
#include "../database.h"

namespace database
{

// AdminRepository implementation
std::optional<Admin> AdminRepository::FindBySteamID(int64_t steam_id)
{
    try
    {
        auto result = Database::Instance().ExecutePrepared("find_admin_by_steamid",
                                                           "SELECT * FROM admins WHERE steam_id = $1", steam_id);

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

bool AdminRepository::Create(const Admin& admin)
{
    try
    {
        // TODO: Implement admin creation
        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool AdminRepository::Update(const Admin& admin)
{
    try
    {
        // TODO: Implement admin update
        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool AdminRepository::Delete(int64_t steam_id)
{
    try
    {
        Database::Instance().ExecutePrepared("delete_admin", "DELETE FROM admins WHERE steam_id = $1", steam_id);

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
    admin.id = row["id"].as<int64_t>();
    admin.steam_id = row["steam_id"].as<int64_t>();
    admin.name = row["name"].c_str();
    admin.flags = row["flags"].c_str();
    admin.immunity = row["immunity"].as<int32_t>();
    admin.created_at = row["created_at"].as<int64_t>();
    admin.updated_at = row["updated_at"].as<int64_t>();

    // Parse groups array (PostgreSQL text array)
    // TODO: Implement proper array parsing
    // For now, leave empty
    admin.groups = {};

    return admin;
}

// AdminGroupRepository implementation
std::optional<AdminGroup> AdminGroupRepository::FindByName(const std::string& name)
{
    try
    {
        auto result = Database::Instance().ExecutePrepared("find_group_by_name",
                                                           "SELECT * FROM admin_groups WHERE name = $1", name);

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

bool AdminGroupRepository::Create(const AdminGroup& group)
{
    try
    {
        // TODO: Implement group creation
        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool AdminGroupRepository::Update(const AdminGroup& group)
{
    try
    {
        // TODO: Implement group update
        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool AdminGroupRepository::Delete(const std::string& name)
{
    try
    {
        Database::Instance().ExecutePrepared("delete_group", "DELETE FROM admin_groups WHERE name = $1", name);

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
    group.id = row["id"].as<int64_t>();
    group.name = row["name"].c_str();
    group.flags = row["flags"].c_str();
    group.immunity = row["immunity"].as<int32_t>();
    group.created_at = row["created_at"].as<int64_t>();
    group.updated_at = row["updated_at"].as<int64_t>();

    // Parse inherits array (PostgreSQL text array)
    // TODO: Implement proper array parsing
    // For now, leave empty
    group.inherits = {};

    return group;
}

}  // namespace database
