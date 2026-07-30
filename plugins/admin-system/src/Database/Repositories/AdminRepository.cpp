#include "AdminRepository.hpp"

#include "../../Core/Managers.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Database/Api.hpp>
#include <pqxx/array>
#include <utility>

namespace AdminSystem::Database
{

namespace
{
/**
 * Parse a libpqxx text-array field (e.g. `{foo,bar,baz}`) into a std::vector<std::string>.
 * Returns empty on null. libpqxx's `array_parser` walks the textual representation token by token.
 * (Array columns are why these two repos keep hand-written ParseRow instead of the Column table.)
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

std::vector<Admin> AdminRepository::FindAll()
{
    auto result = App().Db.QueryBlocking("find_all_admins", "SELECT * FROM admins");
    if (!result)
        return {};

    std::vector<Admin> admins;
    for (const auto& row : *result)
        admins.push_back(ParseRow(row));
    return admins;
}

void AdminRepository::UpdateChatStyle(int64_t steamId, bool displayPrefix, const std::string& nameColor,
                                      const std::string& messageColor)
{
    App().Db.Exec("update_admin_chat_style",
                  "UPDATE admins SET display_prefix = $2, name_color = $3, message_color = $4, "
                  "updated_at = EXTRACT(EPOCH FROM NOW())::BIGINT WHERE steam_id = $1",
                  pqxx::params{steamId, displayPrefix, nameColor, messageColor});
}

void AdminRepository::UpdateLanguage(int64_t steamId, const std::string& lang)
{
    App().Db.Exec("update_admin_language",
                  "UPDATE admins SET language = $2, updated_at = EXTRACT(EPOCH FROM NOW())::BIGINT "
                  "WHERE steam_id = $1",
                  pqxx::params{steamId, lang});
}

bool AdminRepository::SetFrozen(int64_t steamId, int64_t frozenBy, const std::string& reason)
{
    auto result = App().Db.QueryBlocking("set_admin_frozen",
                                         "UPDATE admins SET is_frozen = TRUE, "
                                         "frozen_at = EXTRACT(EPOCH FROM NOW())::BIGINT, frozen_by = $2, "
                                         "freeze_reason = $3, updated_at = EXTRACT(EPOCH FROM NOW())::BIGINT "
                                         "WHERE steam_id = $1",
                                         pqxx::params{steamId, frozenBy, reason});
    return result.has_value();
}

bool AdminRepository::ClearFrozen(int64_t steamId)
{
    auto result = App().Db.QueryBlocking("clear_admin_frozen",
                                         "UPDATE admins SET is_frozen = FALSE, frozen_at = 0, frozen_by = 0, "
                                         "freeze_reason = '', updated_at = EXTRACT(EPOCH FROM NOW())::BIGINT "
                                         "WHERE steam_id = $1",
                                         pqxx::params{steamId});
    return result.has_value();
}

void AdminRepository::FindFrozenAsync(std::function<void(std::vector<FrozenAdmin>)> onDone)
{
    App().Db.Query("find_frozen_admins",
                   "SELECT steam_id, name, frozen_at, frozen_by, freeze_reason FROM admins WHERE is_frozen = TRUE", {},
                   [onDone = std::move(onDone)](CS2Kit::DbResult<pqxx::result> result) {
                       if (!result || !onDone)
                           return;  // DB error already logged; keep the caller's cached set.

                       std::vector<FrozenAdmin> frozen;
                       for (const auto& row : *result)
                       {
                           frozen.push_back({.SteamId = row["steam_id"].as<int64_t>(),
                                             .Name = row["name"].c_str(),
                                             .FrozenAt = row["frozen_at"].as<int64_t>(),
                                             .FrozenBy = row["frozen_by"].as<int64_t>(),
                                             .Reason = row["freeze_reason"].c_str()});
                       }
                       onDone(std::move(frozen));
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

std::vector<AdminGroup> AdminGroupRepository::FindAll()
{
    auto result = App().Db.QueryBlocking("find_all_admin_groups", "SELECT * FROM admin_groups");
    if (!result)
        return {};

    std::vector<AdminGroup> groups;
    for (const auto& row : *result)
        groups.push_back(ParseRow(row));
    return groups;
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
