#include "GagRepository.hpp"

#include "../Database.hpp"

#include <CS2Kit/Utils/TimeUtils.hpp>

namespace AdminSystem::Database
{

using namespace CS2Kit::Utils;

std::optional<Gag> GagRepository::FindActiveBySteamId(int64_t steamId)
{
    try
    {
        auto result = Database::Instance().ExecutePrepared("find_active_gag_by_steamid",
                                                           "SELECT * FROM gags WHERE target_steam_id = $1 AND "
                                                           "is_active = true AND (expires_at = 0 OR expires_at > $2)",
                                                           steamId, TimeUtils::Now());
        if (result.empty())
            return std::nullopt;
        return ParseRow(result[0]);
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

std::vector<Gag> GagRepository::FindAllActive()
{
    std::vector<Gag> gags;
    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "find_all_active_gags", "SELECT * FROM gags WHERE is_active = true AND (expires_at = 0 OR expires_at > $1)",
            TimeUtils::Now());
        for (const auto& row : result)
            gags.push_back(ParseRow(row));
    }
    catch (const std::exception&)
    {
    }
    return gags;
}

bool GagRepository::Create(Gag& gag)
{
    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "create_gag",
            "INSERT INTO gags (target_steam_id, target_name, admin_steam_id, admin_name, reason, "
            "created_at, expires_at, duration, is_active) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) RETURNING id",
            gag.TargetSteamId, gag.TargetName, gag.AdminSteamId, gag.AdminName, gag.Reason, gag.CreatedAt,
            gag.ExpiresAt, gag.Duration, gag.IsActive);
        if (!result.empty())
            gag.Id = result[0]["id"].as<int64_t>();
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool GagRepository::Remove(int64_t gagId, int64_t removedBy, const std::string& reason)
{
    try
    {
        Database::Instance().ExecutePrepared(
            "remove_gag",
            "UPDATE gags SET is_active = false, removed_at = $2, removed_by = $3, removed_reason = $4 WHERE id = $1",
            gagId, TimeUtils::Now(), removedBy, reason);
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

int GagRepository::ExpireOldGags()
{
    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "expire_old_gags",
            "UPDATE gags SET is_active = false WHERE is_active = true AND expires_at > 0 AND expires_at <= $1",
            TimeUtils::Now());
        return result.affected_rows();
    }
    catch (const std::exception&)
    {
        return 0;
    }
}

std::vector<Gag> GagRepository::GetHistory(int64_t steamId)
{
    std::vector<Gag> gags;
    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "get_gag_history", "SELECT * FROM gags WHERE target_steam_id = $1 ORDER BY created_at DESC", steamId);
        for (const auto& row : result)
            gags.push_back(ParseRow(row));
    }
    catch (const std::exception&)
    {
    }
    return gags;
}

Gag GagRepository::ParseRow(const pqxx::row& row)
{
    Gag gag;
    gag.Id = row["id"].as<int64_t>();
    gag.TargetSteamId = row["target_steam_id"].as<int64_t>();
    gag.TargetName = row["target_name"].c_str();
    gag.AdminSteamId = row["admin_steam_id"].as<int64_t>();
    gag.AdminName = row["admin_name"].c_str();
    gag.Reason = row["reason"].c_str();
    gag.CreatedAt = row["created_at"].as<int64_t>();
    gag.ExpiresAt = row["expires_at"].as<int64_t>();
    gag.Duration = row["duration"].as<int64_t>();
    gag.IsActive = row["is_active"].as<bool>();
    if (!row["removed_at"].is_null())
    {
        gag.RemovedAt = row["removed_at"].as<int64_t>();
        gag.RemovedBy = row["removed_by"].as<int64_t>();
        gag.RemovedReason = row["removed_reason"].c_str();
    }
    return gag;
}

}  // namespace AdminSystem::Database
