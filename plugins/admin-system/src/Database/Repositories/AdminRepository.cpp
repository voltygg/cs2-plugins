#include "AdminRepository.hpp"

#include "../../Core/Managers.hpp"

#include <CS2Kit/Database/DbResult.hpp>
#include <CS2Kit/Database/PostgresDatabase.hpp>
#include <pqxx/array>

namespace AdminSystem::Database
{

using CS2Kit::Database::TryOr;

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
    return TryOr<std::optional<Admin>>(std::nullopt, "AdminRepository::FindBySteamId", [&]() -> std::optional<Admin> {
        auto result =
            App().Db.ExecutePrepared("find_admin_by_steamid", "SELECT * FROM admins WHERE steam_id = $1", steamId);

        if (result.empty())
        {
            return std::nullopt;
        }

        return ParseRow(result[0]);
    });
}

std::vector<Admin> AdminRepository::FindAll()
{
    return TryOr(std::vector<Admin>{}, "AdminRepository::FindAll", [&] {
        std::vector<Admin> admins;
        auto result = App().Db.Execute("SELECT * FROM admins");

        for (const auto& row : result)
        {
            admins.push_back(ParseRow(row));
        }
        return admins;
    });
}

bool AdminRepository::Delete(int64_t steamId)
{
    return TryOr(false, "AdminRepository::Delete", [&] {
        App().Db.ExecutePrepared("delete_admin", "DELETE FROM admins WHERE steam_id = $1", steamId);

        return true;
    });
}

bool AdminRepository::UpdateChatStyle(int64_t steamId, bool displayPrefix, const std::string& nameColor,
                                      const std::string& messageColor)
{
    return TryOr(false, "AdminRepository::UpdateChatStyle", [&] {
        App().Db.ExecutePrepared("update_admin_chat_style",
                                 "UPDATE admins SET display_prefix = $2, name_color = $3, message_color = $4, "
                                 "updated_at = EXTRACT(EPOCH FROM NOW())::BIGINT WHERE steam_id = $1",
                                 steamId, displayPrefix, nameColor, messageColor);
        return true;
    });
}

bool AdminRepository::UpdateLanguage(int64_t steamId, const std::string& lang)
{
    return TryOr(false, "AdminRepository::UpdateLanguage", [&] {
        App().Db.ExecutePrepared("update_admin_language",
                                 "UPDATE admins SET language = $2, updated_at = EXTRACT(EPOCH FROM NOW())::BIGINT "
                                 "WHERE steam_id = $1",
                                 steamId, lang);
        return true;
    });
}

bool AdminRepository::SetFrozen(int64_t steamId, int64_t frozenBy, const std::string& reason)
{
    return TryOr(false, "AdminRepository::SetFrozen", [&] {
        App().Db.ExecutePrepared("set_admin_frozen",
                                 "UPDATE admins SET is_frozen = TRUE, "
                                 "frozen_at = EXTRACT(EPOCH FROM NOW())::BIGINT, frozen_by = $2, "
                                 "freeze_reason = $3, updated_at = EXTRACT(EPOCH FROM NOW())::BIGINT "
                                 "WHERE steam_id = $1",
                                 steamId, frozenBy, reason);
        return true;
    });
}

bool AdminRepository::ClearFrozen(int64_t steamId)
{
    return TryOr(false, "AdminRepository::ClearFrozen", [&] {
        App().Db.ExecutePrepared("clear_admin_frozen",
                                 "UPDATE admins SET is_frozen = FALSE, frozen_at = 0, frozen_by = 0, "
                                 "freeze_reason = '', updated_at = EXTRACT(EPOCH FROM NOW())::BIGINT "
                                 "WHERE steam_id = $1",
                                 steamId);
        return true;
    });
}

CS2Kit::Database::DbResult<std::vector<FrozenAdmin>> AdminRepository::FindFrozen()
{
    return CS2Kit::Database::TryDb("AdminRepository::FindFrozen", [&] {
        std::vector<FrozenAdmin> frozen;
        auto result = App().Db.ExecutePrepared(
            "find_frozen_admins",
            "SELECT steam_id, name, frozen_at, frozen_by, freeze_reason FROM admins WHERE is_frozen = TRUE");

        for (const auto& row : result)
        {
            frozen.push_back({.SteamId = row["steam_id"].as<int64_t>(),
                              .Name = row["name"].c_str(),
                              .FrozenAt = row["frozen_at"].as<int64_t>(),
                              .FrozenBy = row["frozen_by"].as<int64_t>(),
                              .Reason = row["freeze_reason"].c_str()});
        }
        return frozen;
    });
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
    return TryOr<std::optional<AdminGroup>>(
        std::nullopt, "AdminGroupRepository::FindByName", [&]() -> std::optional<AdminGroup> {
            auto result =
                App().Db.ExecutePrepared("find_group_by_name", "SELECT * FROM admin_groups WHERE name = $1", name);

            if (result.empty())
            {
                return std::nullopt;
            }

            return ParseRow(result[0]);
        });
}

std::vector<AdminGroup> AdminGroupRepository::FindAll()
{
    return TryOr(std::vector<AdminGroup>{}, "AdminGroupRepository::FindAll", [&] {
        std::vector<AdminGroup> groups;
        auto result = App().Db.Execute("SELECT * FROM admin_groups");

        for (const auto& row : result)
        {
            groups.push_back(ParseRow(row));
        }
        return groups;
    });
}

bool AdminGroupRepository::Delete(const std::string& name)
{
    return TryOr(false, "AdminGroupRepository::Delete", [&] {
        App().Db.ExecutePrepared("delete_group", "DELETE FROM admin_groups WHERE name = $1", name);

        return true;
    });
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
