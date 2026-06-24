#include "WarningRepository.hpp"
#include "../../Core/Managers.hpp"

#include "../Database.hpp"

#include <CS2Kit/Database/DbResult.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>

namespace AdminSystem::Database
{

using namespace CS2Kit::Utils;
using CS2Kit::Database::TryOr;

bool WarningRepository::Create(Warning& warning)
{
    return TryOr(false, "WarningRepository::Create", [&] {
        auto result = Sys().Db.ExecutePrepared(
            "create_warning",
            "INSERT INTO warnings (target_steam_id, target_name, admin_steam_id, admin_name, reason, "
            "created_at, is_active, expires_at) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) RETURNING id",
            warning.TargetSteamId, warning.TargetName, warning.AdminSteamId, warning.AdminName, warning.Reason,
            warning.CreatedAt, warning.IsActive, warning.ExpiresAt);
        if (!result.empty())
            warning.Id = result[0]["id"].as<int64_t>();
        return true;
    });
}

int WarningRepository::CountActive(int64_t steamId)
{
    return TryOr(0, "WarningRepository::CountActive", [&]() -> int {
        auto result = Sys().Db.ExecutePrepared(
            "count_active_warnings",
            "SELECT COUNT(*) AS total FROM warnings WHERE target_steam_id = $1 AND is_active = true "
            "AND (expires_at = 0 OR expires_at > $2)",
            steamId, TimeUtils::Now());
        if (result.empty())
            return 0;
        return result[0]["total"].as<int>();
    });
}

int WarningRepository::Clear(int64_t steamId)
{
    return TryOr(0, "WarningRepository::Clear", [&]() -> int {
        auto result = Sys().Db.ExecutePrepared(
            "clear_warnings", "UPDATE warnings SET is_active = false WHERE target_steam_id = $1 AND is_active = true",
            steamId);
        return result.affected_rows();
    });
}

std::vector<Warning> WarningRepository::GetHistory(int64_t steamId)
{
    return TryOr(std::vector<Warning>{}, "WarningRepository::GetHistory", [&] {
        std::vector<Warning> warnings;
        auto result = Sys().Db.ExecutePrepared(
            "get_warning_history", "SELECT * FROM warnings WHERE target_steam_id = $1 ORDER BY created_at DESC",
            steamId);
        for (const auto& row : result)
            warnings.push_back(ParseRow(row));
        return warnings;
    });
}

Warning WarningRepository::ParseRow(const pqxx::row& row)
{
    Warning warning;
    warning.Id = row["id"].as<int64_t>();
    warning.TargetSteamId = row["target_steam_id"].as<int64_t>();
    warning.TargetName = row["target_name"].c_str();
    warning.AdminSteamId = row["admin_steam_id"].as<int64_t>();
    warning.AdminName = row["admin_name"].c_str();
    warning.Reason = row["reason"].c_str();
    warning.CreatedAt = row["created_at"].as<int64_t>();
    warning.IsActive = row["is_active"].as<bool>();
    warning.ExpiresAt = row["expires_at"].as<int64_t>();
    return warning;
}

}  // namespace AdminSystem::Database
