#include "MuteRepository.hpp"

#include "../Database.hpp"

#include <CS2Kit/Utils/TimeUtils.hpp>

namespace AdminSystem::Database
{

using namespace CS2Kit::Utils;

std::optional<Mute> MuteRepository::FindActiveBySteamId(int64_t steamId)
{
    try
    {
        auto result = Database::Instance().ExecutePrepared("find_active_mute_by_steamid",
                                                           "SELECT * FROM mutes WHERE target_steam_id = $1 AND "
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

std::vector<Mute> MuteRepository::FindAllActive()
{
    std::vector<Mute> mutes;
    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "find_all_active_mutes", "SELECT * FROM mutes WHERE is_active = true AND (expires_at = 0 OR expires_at > $1)",
            TimeUtils::Now());
        for (const auto& row : result)
            mutes.push_back(ParseRow(row));
    }
    catch (const std::exception&)
    {
    }
    return mutes;
}

bool MuteRepository::Create(Mute& mute)
{
    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "create_mute",
            "INSERT INTO mutes (target_steam_id, target_name, admin_steam_id, admin_name, reason, "
            "created_at, expires_at, duration, is_active) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) RETURNING id",
            mute.TargetSteamId, mute.TargetName, mute.AdminSteamId, mute.AdminName, mute.Reason, mute.CreatedAt,
            mute.ExpiresAt, mute.Duration, mute.IsActive);
        if (!result.empty())
            mute.Id = result[0]["id"].as<int64_t>();
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool MuteRepository::Remove(int64_t muteId, int64_t removedBy, const std::string& reason)
{
    try
    {
        Database::Instance().ExecutePrepared(
            "remove_mute",
            "UPDATE mutes SET is_active = false, removed_at = $2, removed_by = $3, removed_reason = $4 WHERE id = $1",
            muteId, TimeUtils::Now(), removedBy, reason);
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

int MuteRepository::ExpireOldMutes()
{
    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "expire_old_mutes",
            "UPDATE mutes SET is_active = false WHERE is_active = true AND expires_at > 0 AND expires_at <= $1",
            TimeUtils::Now());
        return result.affected_rows();
    }
    catch (const std::exception&)
    {
        return 0;
    }
}

std::vector<Mute> MuteRepository::GetHistory(int64_t steamId)
{
    std::vector<Mute> mutes;
    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "get_mute_history", "SELECT * FROM mutes WHERE target_steam_id = $1 ORDER BY created_at DESC", steamId);
        for (const auto& row : result)
            mutes.push_back(ParseRow(row));
    }
    catch (const std::exception&)
    {
    }
    return mutes;
}

Mute MuteRepository::ParseRow(const pqxx::row& row)
{
    Mute mute;
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

}  // namespace AdminSystem::Database
