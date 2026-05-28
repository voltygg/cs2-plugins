#include "AdminRepository.hpp"

#include "../Database.hpp"

#include <pqxx/array>

namespace AdminSystem::Database
{

namespace
{
/**
 * Parse a libpqxx text-array field (e.g. `{foo,bar,baz}`) into a std::vector<std::string>.
 * Returns empty on null. libpqxx's `array_parser` walks the textual representation token by token.
 */
std::vector<std::string> ParseTextArray(const pqxx::field& field)
{
    std::vector<std::string> out;
    if (field.is_null())
        return out;

    pqxx::array_parser parser(field.c_str());
    while (true)
    {
        auto [type, value] = parser.get_next();
        if (type == pqxx::array_parser::juncture::done)
            break;
        if (type == pqxx::array_parser::juncture::string_value)
            out.push_back(std::move(value));
    }
    return out;
}
}  // namespace

// AdminRepository implementation

std::optional<Admin> AdminRepository::FindBySteamId(int64_t steamId)
{
    try
    {
        auto result = Database::Instance().ExecutePrepared("find_admin_by_steamid",
                                                           "SELECT * FROM admins WHERE steam_id = $1", steamId);

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
        Database::Instance().ExecutePrepared("delete_admin", "DELETE FROM admins WHERE steam_id = $1", steamId);

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool AdminRepository::UpdateChatStyle(int64_t steamId, bool displayPrefix, const std::string& nameColor,
                                       const std::string& messageColor)
{
    try
    {
        Database::Instance().ExecutePrepared(
            "update_admin_chat_style",
            "UPDATE admins SET display_prefix = $2, name_color = $3, message_color = $4, "
            "updated_at = EXTRACT(EPOCH FROM NOW())::BIGINT WHERE steam_id = $1",
            steamId, displayPrefix, nameColor, messageColor);
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool AdminRepository::UpdateLanguage(int64_t steamId, const std::string& lang)
{
    try
    {
        Database::Instance().ExecutePrepared(
            "update_admin_language",
            "UPDATE admins SET language = $2, updated_at = EXTRACT(EPOCH FROM NOW())::BIGINT "
            "WHERE steam_id = $1",
            steamId, lang);
        return true;
    }
    catch (const std::exception&)
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
    admin.Groups = ParseTextArray(row["groups"]);
    admin.DisplayPrefix = row["display_prefix"].as<bool>(true);
    admin.NameColor = row["name_color"].c_str();
    admin.MessageColor = row["message_color"].c_str();
    admin.Language = row["language"].c_str();

    admin.BuildFlagBits();
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
    group.Id = row["id"].as<int64_t>();
    group.Name = row["name"].c_str();
    group.Flags = row["flags"].c_str();
    group.Immunity = row["immunity"].as<int32_t>();
    group.CreatedAt = row["created_at"].as<int64_t>();
    group.UpdatedAt = row["updated_at"].as<int64_t>();
    group.Inherits = ParseTextArray(row["inherits"]);
    group.ChatPrefix = row["chat_prefix"].c_str();
    group.PrefixColor = row["prefix_color"].c_str();
    group.NameColor = row["name_color"].c_str();
    group.MessageColor = row["message_color"].c_str();

    group.BuildFlagBits();
    return group;
}

}  // namespace AdminSystem::Database
