#pragma once

#include <VoltMod/Database/Table.hpp>
#include <optional>
#include <string>

namespace AdminSystem::Database::Tables
{

struct Bans_
{
    VOLTMOD_COLUMN(id, id, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(targetSteamId, target_steam_id, sqlpp::integral, std::false_type);
    VOLTMOD_COLUMN(targetName, target_name, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(targetIp, target_ip, std::optional<sqlpp::text>, std::true_type);
    VOLTMOD_COLUMN(adminSteamId, admin_steam_id, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(adminName, admin_name, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(reason, reason, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(createdAt, created_at, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(expiresAt, expires_at, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(duration, duration, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(isActive, is_active, sqlpp::boolean, std::true_type);
    VOLTMOD_COLUMN(removedAt, removed_at, std::optional<sqlpp::integral>, std::true_type);
    VOLTMOD_COLUMN(removedBy, removed_by, std::optional<sqlpp::integral>, std::true_type);
    VOLTMOD_COLUMN(removedReason, removed_reason, std::optional<sqlpp::text>, std::true_type);

    SQLPP_CREATE_NAME_TAG_FOR_SQL_AND_CPP(bans, bans);
    template <typename T>
    using _table_columns = sqlpp::table_columns<T, id, targetSteamId, targetName, targetIp, adminSteamId, adminName,
                                                 reason, createdAt, expiresAt, duration, isActive, removedAt,
                                                 removedBy, removedReason>;
    using _required_insert_columns = sqlpp::detail::type_set<sqlpp::column_t<sqlpp::table_t<Bans_>, targetSteamId>,
                                                              sqlpp::column_t<sqlpp::table_t<Bans_>, targetName>,
                                                              sqlpp::column_t<sqlpp::table_t<Bans_>, adminName>,
                                                              sqlpp::column_t<sqlpp::table_t<Bans_>, reason>>;
};
using Bans = sqlpp::table_t<Bans_>;

struct VoiceMutes_
{
    VOLTMOD_COLUMN(id, id, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(targetSteamId, target_steam_id, sqlpp::integral, std::false_type);
    VOLTMOD_COLUMN(targetName, target_name, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(adminSteamId, admin_steam_id, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(adminName, admin_name, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(reason, reason, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(createdAt, created_at, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(expiresAt, expires_at, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(duration, duration, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(isActive, is_active, sqlpp::boolean, std::true_type);
    VOLTMOD_COLUMN(removedAt, removed_at, std::optional<sqlpp::integral>, std::true_type);
    VOLTMOD_COLUMN(removedBy, removed_by, std::optional<sqlpp::integral>, std::true_type);
    VOLTMOD_COLUMN(removedReason, removed_reason, std::optional<sqlpp::text>, std::true_type);

    SQLPP_CREATE_NAME_TAG_FOR_SQL_AND_CPP(voice_mutes, voice_mutes);
    template <typename T>
    using _table_columns = sqlpp::table_columns<T, id, targetSteamId, targetName, adminSteamId, adminName, reason,
                                                 createdAt, expiresAt, duration, isActive, removedAt, removedBy,
                                                 removedReason>;
    using _required_insert_columns =
        sqlpp::detail::type_set<sqlpp::column_t<sqlpp::table_t<VoiceMutes_>, targetSteamId>,
                                 sqlpp::column_t<sqlpp::table_t<VoiceMutes_>, targetName>,
                                 sqlpp::column_t<sqlpp::table_t<VoiceMutes_>, adminName>,
                                 sqlpp::column_t<sqlpp::table_t<VoiceMutes_>, reason>>;
};
using VoiceMutes = sqlpp::table_t<VoiceMutes_>;

struct TextMutes_
{
    VOLTMOD_COLUMN(id, id, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(targetSteamId, target_steam_id, sqlpp::integral, std::false_type);
    VOLTMOD_COLUMN(targetName, target_name, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(adminSteamId, admin_steam_id, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(adminName, admin_name, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(reason, reason, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(createdAt, created_at, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(expiresAt, expires_at, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(duration, duration, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(isActive, is_active, sqlpp::boolean, std::true_type);
    VOLTMOD_COLUMN(removedAt, removed_at, std::optional<sqlpp::integral>, std::true_type);
    VOLTMOD_COLUMN(removedBy, removed_by, std::optional<sqlpp::integral>, std::true_type);
    VOLTMOD_COLUMN(removedReason, removed_reason, std::optional<sqlpp::text>, std::true_type);

    SQLPP_CREATE_NAME_TAG_FOR_SQL_AND_CPP(text_mutes, text_mutes);
    template <typename T>
    using _table_columns = sqlpp::table_columns<T, id, targetSteamId, targetName, adminSteamId, adminName, reason,
                                                 createdAt, expiresAt, duration, isActive, removedAt, removedBy,
                                                 removedReason>;
    using _required_insert_columns =
        sqlpp::detail::type_set<sqlpp::column_t<sqlpp::table_t<TextMutes_>, targetSteamId>,
                                 sqlpp::column_t<sqlpp::table_t<TextMutes_>, targetName>,
                                 sqlpp::column_t<sqlpp::table_t<TextMutes_>, adminName>,
                                 sqlpp::column_t<sqlpp::table_t<TextMutes_>, reason>>;
};
using TextMutes = sqlpp::table_t<TextMutes_>;

struct Warnings_
{
    VOLTMOD_COLUMN(id, id, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(targetSteamId, target_steam_id, sqlpp::integral, std::false_type);
    VOLTMOD_COLUMN(targetName, target_name, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(adminSteamId, admin_steam_id, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(adminName, admin_name, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(reason, reason, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(createdAt, created_at, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(isActive, is_active, sqlpp::boolean, std::true_type);
    VOLTMOD_COLUMN(expiresAt, expires_at, std::optional<sqlpp::integral>, std::true_type);

    SQLPP_CREATE_NAME_TAG_FOR_SQL_AND_CPP(warnings, warnings);
    template <typename T>
    using _table_columns =
        sqlpp::table_columns<T, id, targetSteamId, targetName, adminSteamId, adminName, reason, createdAt, isActive,
                              expiresAt>;
    using _required_insert_columns = sqlpp::detail::type_set<sqlpp::column_t<sqlpp::table_t<Warnings_>, targetSteamId>,
                                                              sqlpp::column_t<sqlpp::table_t<Warnings_>, targetName>,
                                                              sqlpp::column_t<sqlpp::table_t<Warnings_>, adminName>,
                                                              sqlpp::column_t<sqlpp::table_t<Warnings_>, reason>>;
};
using Warnings = sqlpp::table_t<Warnings_>;

/** Shared row shape of Ban/VoiceMute/TextMute; @p Ban alone also carries target_ip. */
template <class Entity, class Row>
Entity PunishmentFromRow(const Row& row)
{
    Entity entity;
    entity.Id = row.id;
    entity.TargetSteamId = row.targetSteamId;
    entity.TargetName = row.targetName;
    if constexpr (requires { entity.TargetIp = row.targetIp.value_or(std::string{}); })
        entity.TargetIp = row.targetIp.value_or(std::string{});
    entity.AdminSteamId = row.adminSteamId;
    entity.AdminName = row.adminName;
    entity.Reason = row.reason;
    entity.CreatedAt = row.createdAt;
    entity.ExpiresAt = row.expiresAt;
    entity.Duration = row.duration;
    entity.IsActive = row.isActive;
    entity.RemovedAt = row.removedAt;
    entity.RemovedBy = row.removedBy;
    entity.RemovedReason = row.removedReason;
    return entity;
}

template <class Table, class Entity>
auto InsertPunishment(const Table& t, const Entity& e)
{
    if constexpr (requires { t.targetIp; e.TargetIp; })
    {
        return sqlpp::insert_into(t).set(t.targetSteamId = e.TargetSteamId, t.targetName = e.TargetName,
                                          t.targetIp = e.TargetIp, t.adminSteamId = e.AdminSteamId,
                                          t.adminName = e.AdminName, t.reason = e.Reason, t.createdAt = e.CreatedAt,
                                          t.expiresAt = e.ExpiresAt, t.duration = e.Duration, t.isActive = e.IsActive);
    }
    else
    {
        return sqlpp::insert_into(t).set(t.targetSteamId = e.TargetSteamId, t.targetName = e.TargetName,
                                          t.adminSteamId = e.AdminSteamId, t.adminName = e.AdminName,
                                          t.reason = e.Reason, t.createdAt = e.CreatedAt, t.expiresAt = e.ExpiresAt,
                                          t.duration = e.Duration, t.isActive = e.IsActive);
    }
}

template <class Entity>
auto InsertWarning(const Warnings& t, const Entity& e)
{
    return sqlpp::insert_into(t).set(t.targetSteamId = e.TargetSteamId, t.targetName = e.TargetName,
                                      t.adminSteamId = e.AdminSteamId, t.adminName = e.AdminName, t.reason = e.Reason,
                                      t.createdAt = e.CreatedAt, t.isActive = e.IsActive, t.expiresAt = e.ExpiresAt);
}

}  // namespace AdminSystem::Database::Tables
