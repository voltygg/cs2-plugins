#pragma once

#include <VoltMod/Database/Table.hpp>
#include <optional>

namespace AdminSystem::Database::Tables
{

struct AdminGroups_
{
    VOLTMOD_COLUMN(id, id, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(name, name, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(flags, flags, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(immunity, immunity, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(inherits, inherits, sqlpp::text, std::true_type);  // JSON array text
    VOLTMOD_COLUMN(chatPrefix, chat_prefix, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(prefixColor, prefix_color, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(nameColor, name_color, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(messageColor, message_color, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(createdAt, created_at, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(updatedAt, updated_at, sqlpp::integral, std::true_type);

    SQLPP_CREATE_NAME_TAG_FOR_SQL_AND_CPP(admin_groups, admin_groups);
    template <typename T>
    using _table_columns = sqlpp::table_columns<T, id, name, flags, immunity, inherits, chatPrefix, prefixColor,
                                                 nameColor, messageColor, createdAt, updatedAt>;
    using _required_insert_columns = sqlpp::detail::type_set<sqlpp::column_t<sqlpp::table_t<AdminGroups_>, name>>;
};
using AdminGroups = sqlpp::table_t<AdminGroups_>;

struct Admins_
{
    VOLTMOD_COLUMN(id, id, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(steamId, steam_id, sqlpp::integral, std::false_type);
    VOLTMOD_COLUMN(name, name, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(groups, groups, sqlpp::text, std::true_type);  // JSON array text
    VOLTMOD_COLUMN(flags, flags, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(immunity, immunity, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(displayPrefix, display_prefix, sqlpp::boolean, std::true_type);
    VOLTMOD_COLUMN(nameColor, name_color, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(messageColor, message_color, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(language, language, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(createdAt, created_at, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(updatedAt, updated_at, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(isFrozen, is_frozen, sqlpp::boolean, std::true_type);
    VOLTMOD_COLUMN(frozenAt, frozen_at, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(frozenBy, frozen_by, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(freezeReason, freeze_reason, sqlpp::text, std::true_type);

    SQLPP_CREATE_NAME_TAG_FOR_SQL_AND_CPP(admins, admins);
    template <typename T>
    using _table_columns =
        sqlpp::table_columns<T, id, steamId, name, groups, flags, immunity, displayPrefix, nameColor, messageColor,
                              language, createdAt, updatedAt, isFrozen, frozenAt, frozenBy, freezeReason>;
    using _required_insert_columns = sqlpp::detail::type_set<sqlpp::column_t<sqlpp::table_t<Admins_>, steamId>,
                                                              sqlpp::column_t<sqlpp::table_t<Admins_>, name>>;
};
using Admins = sqlpp::table_t<Admins_>;

struct AdminServerGroups_
{
    VOLTMOD_COLUMN(id, id, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(adminSteamId, admin_steam_id, sqlpp::integral, std::false_type);
    VOLTMOD_COLUMN(serverTag, server_tag, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(groupName, group_name, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(createdAt, created_at, sqlpp::integral, std::true_type);

    SQLPP_CREATE_NAME_TAG_FOR_SQL_AND_CPP(admin_server_groups, admin_server_groups);
    template <typename T>
    using _table_columns = sqlpp::table_columns<T, id, adminSteamId, serverTag, groupName, createdAt>;
    using _required_insert_columns =
        sqlpp::detail::type_set<sqlpp::column_t<sqlpp::table_t<AdminServerGroups_>, adminSteamId>,
                                 sqlpp::column_t<sqlpp::table_t<AdminServerGroups_>, serverTag>,
                                 sqlpp::column_t<sqlpp::table_t<AdminServerGroups_>, groupName>>;
};
using AdminServerGroups = sqlpp::table_t<AdminServerGroups_>;

struct AdminActivity_
{
    VOLTMOD_COLUMN(id, id, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(adminSteamId, admin_steam_id, sqlpp::integral, std::false_type);
    VOLTMOD_COLUMN(adminName, admin_name, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(action, action, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(targetSteamId, target_steam_id, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(targetName, target_name, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(detail, detail, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(serverTag, server_tag, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(createdAt, created_at, sqlpp::integral, std::true_type);

    SQLPP_CREATE_NAME_TAG_FOR_SQL_AND_CPP(admin_activity, admin_activity);
    template <typename T>
    using _table_columns = sqlpp::table_columns<T, id, adminSteamId, adminName, action, targetSteamId, targetName,
                                                 detail, serverTag, createdAt>;
    using _required_insert_columns =
        sqlpp::detail::type_set<sqlpp::column_t<sqlpp::table_t<AdminActivity_>, adminSteamId>,
                                 sqlpp::column_t<sqlpp::table_t<AdminActivity_>, action>>;
};
using AdminActivity = sqlpp::table_t<AdminActivity_>;

struct Servers_
{
    VOLTMOD_COLUMN(id, id, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(tag, tag, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(name, name, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(createdAt, created_at, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(lastSeen, last_seen, sqlpp::integral, std::true_type);

    SQLPP_CREATE_NAME_TAG_FOR_SQL_AND_CPP(servers, servers);
    template <typename T>
    using _table_columns = sqlpp::table_columns<T, id, tag, name, createdAt, lastSeen>;
    using _required_insert_columns = sqlpp::detail::type_set<sqlpp::column_t<sqlpp::table_t<Servers_>, tag>>;
};
using Servers = sqlpp::table_t<Servers_>;

}  // namespace AdminSystem::Database::Tables
