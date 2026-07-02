#pragma once

#include "../../Core/Managers.hpp"

#include <CS2Kit/Database/DbResult.hpp>
#include <CS2Kit/Database/PostgresDatabase.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>
#include <format>
#include <optional>
#include <pqxx/pqxx>
#include <string>
#include <vector>

namespace AdminSystem::Database
{

/**
 * Repository for mute records - lookup, creation, removal, expiration, and history.
 * Templated over the entity (VoiceMute / TextMute) since both tables share an identical
 * schema and row mapping. The table name and a prepared-statement-name prefix are passed in
 * at construction so the two tables keep DISTINCT prepared statements (e.g. "voice_mute" vs
 * "text_mute").
 */
template <typename TEntity>
class MuteRepository
{
public:
    MuteRepository(std::string table, std::string statementPrefix)
        : _table(std::move(table)), _prefix(std::move(statementPrefix))
    {}

    std::optional<TEntity> FindActiveBySteamId(int64_t steamId)
    {
        return CS2Kit::Database::TryOr<std::optional<TEntity>>(
            std::nullopt, Label("FindActiveBySteamId"), [&]() -> std::optional<TEntity> {
                auto result =
                    App().Db.ExecutePrepared(Stmt("find_active_by_steamid"),
                                             std::format("SELECT * FROM {} WHERE target_steam_id = $1 AND "
                                                         "is_active = true AND (expires_at = 0 OR expires_at > $2)",
                                                         _table),
                                             steamId, CS2Kit::Utils::TimeUtils::Now());
                if (result.empty())
                    return std::nullopt;
                return ParseRow(result[0]);
            });
    }

    std::vector<TEntity> FindAllActive()
    {
        return CS2Kit::Database::TryOr(std::vector<TEntity>{}, Label("FindAllActive"), [&] {
            std::vector<TEntity> mutes;
            auto result = App().Db.ExecutePrepared(
                Stmt("find_all_active"),
                std::format("SELECT * FROM {} WHERE is_active = true AND (expires_at = 0 OR expires_at > $1)", _table),
                CS2Kit::Utils::TimeUtils::Now());
            for (const auto& row : result)
                mutes.push_back(ParseRow(row));
            return mutes;
        });
    }

    bool Create(TEntity& mute)
    {
        return CS2Kit::Database::TryOr(false, Label("Create"), [&] {
            auto result = App().Db.ExecutePrepared(
                Stmt("create"),
                std::format("INSERT INTO {} (target_steam_id, target_name, admin_steam_id, admin_name, reason, "
                            "created_at, expires_at, duration, is_active) "
                            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) RETURNING id",
                            _table),
                mute.TargetSteamId, mute.TargetName, mute.AdminSteamId, mute.AdminName, mute.Reason, mute.CreatedAt,
                mute.ExpiresAt, mute.Duration, mute.IsActive);
            if (!result.empty())
                mute.Id = result[0]["id"].template as<int64_t>();
            return true;
        });
    }

    bool Remove(int64_t muteId, int64_t removedBy, const std::string& reason)
    {
        return CS2Kit::Database::TryOr(false, Label("Remove"), [&] {
            App().Db.ExecutePrepared(
                Stmt("remove"),
                std::format("UPDATE {} SET is_active = false, removed_at = $2, removed_by = $3, removed_reason = $4 "
                            "WHERE id = $1",
                            _table),
                muteId, CS2Kit::Utils::TimeUtils::Now(), removedBy, reason);
            return true;
        });
    }

    int ExpireOld()
    {
        return CS2Kit::Database::TryOr(0, Label("ExpireOld"), [&]() -> int {
            auto result = App().Db.ExecutePrepared(
                Stmt("expire_old"),
                std::format("UPDATE {} SET is_active = false WHERE is_active = true AND expires_at > 0 AND "
                            "expires_at <= $1",
                            _table),
                CS2Kit::Utils::TimeUtils::Now());
            return result.affected_rows();
        });
    }

    std::vector<TEntity> GetHistory(int64_t steamId)
    {
        return CS2Kit::Database::TryOr(std::vector<TEntity>{}, Label("GetHistory"), [&] {
            std::vector<TEntity> mutes;
            auto result = App().Db.ExecutePrepared(
                Stmt("get_history"),
                std::format("SELECT * FROM {} WHERE target_steam_id = $1 ORDER BY created_at DESC", _table), steamId);
            for (const auto& row : result)
                mutes.push_back(ParseRow(row));
            return mutes;
        });
    }

private:
    // Distinct prepared-statement name per table, e.g. "voice_mute_find_active_by_steamid".
    std::string Stmt(std::string_view op) const { return std::format("{}_{}", _prefix, op); }

    // Error-context label distinguishing the two repos in logs, e.g. "voice_mute::Create".
    std::string Label(std::string_view method) const { return std::format("{}::{}", _prefix, method); }

    TEntity ParseRow(const pqxx::row& row)
    {
        TEntity mute;
        mute.Id = row["id"].as<int64_t>();
        mute.TargetSteamId = row["target_steam_id"].as<int64_t>();
        mute.TargetName = row["target_name"].c_str();
        mute.AdminSteamId = row["admin_steam_id"].as<int64_t>();
        mute.AdminName = row["admin_name"].c_str();
        mute.Reason = row["reason"].c_str();
        mute.CreatedAt = row["created_at"].as<int64_t>();
        mute.ExpiresAt = row["expires_at"].as<int64_t>();
        mute.Duration = row["duration"].as<int64_t>();
        mute.IsActive = row["is_active"].as<bool>();
        if (!row["removed_at"].is_null())
        {
            mute.RemovedAt = row["removed_at"].as<int64_t>();
            mute.RemovedBy = row["removed_by"].as<int64_t>();
            mute.RemovedReason = row["removed_reason"].c_str();
        }
        return mute;
    }

    std::string _table;
    std::string _prefix;
};

}  // namespace AdminSystem::Database
