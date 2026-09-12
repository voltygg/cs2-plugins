#pragma once

#include "Tables/Schema.hpp"

#include <sqlpp23/sqlpp23.h>
#include <string>

namespace AdminSystem::Database
{

// Names the COUNT column so the row reads as row.total.
SQLPP_CREATE_NAME_TAG(total);

/** Bans, voice mutes and text mutes share one row shape; only a ban carries target_ip. */
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
auto InsertWarning(const Tables::Warnings& t, const Entity& e)
{
    return sqlpp::insert_into(t).set(t.targetSteamId = e.TargetSteamId, t.targetName = e.TargetName,
                                     t.adminSteamId = e.AdminSteamId, t.adminName = e.AdminName, t.reason = e.Reason,
                                     t.createdAt = e.CreatedAt, t.isActive = e.IsActive, t.expiresAt = e.ExpiresAt);
}

template <class Entity>
auto InsertReport(const Tables::PlayerReports& t, const Entity& e)
{
    return sqlpp::insert_into(t).set(t.reporterSteamId = e.ReporterSteamId, t.reporterName = e.ReporterName,
                                     t.reporterIp = e.ReporterIp, t.targetSteamId = e.TargetSteamId,
                                     t.targetName = e.TargetName, t.targetIp = e.TargetIp, t.reasonCode = e.ReasonCode,
                                     t.reason = e.Reason, t.serverTag = e.ServerTag, t.mapName = e.MapName,
                                     t.createdAt = e.CreatedAt);
}

}  // namespace AdminSystem::Database
