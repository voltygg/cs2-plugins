#include "BanRepository.hpp"
#include "../../Utils/TimeUtils.hpp"
#include "../Database.hpp"

namespace AdminSystem::Database {

std::optional<Ban> BanRepository::FindActiveBySteamId(int64_t steamId)
{
    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "find_active_ban_by_steamid",
            "SELECT * FROM bans WHERE target_steam_id = $1 AND "
            "is_active = true AND (expires_at = 0 OR expires_at > $2)",
            steamId, Utils::TimeUtils::Now());

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

std::optional<Ban> BanRepository::FindActiveByIp(const std::string& ip)
{
    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "find_active_ban_by_ip",
            "SELECT * FROM bans WHERE target_ip = $1 AND "
            "is_active = true AND (expires_at = 0 OR expires_at > $2)",
            ip, Utils::TimeUtils::Now());

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

std::vector<Ban> BanRepository::FindAllActive()
{
    std::vector<Ban> bans;

    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "find_all_active_bans",
            "SELECT * FROM bans WHERE is_active = true AND (expires_at = 0 OR expires_at > $1)",
            Utils::TimeUtils::Now());

        for (const auto& row : result)
        {
            bans.push_back(ParseRow(row));
        }
    }
    catch (const std::exception& e)
    {
        // Log error
    }

    return bans;
}

bool BanRepository::Create(const Ban& ban)
{
    try
    {
        Database::Instance().ExecutePrepared(
            "create_ban",
            "INSERT INTO bans (target_steam_id, target_name, target_ip, admin_steam_id, admin_name, reason, "
            "created_at, expires_at, duration, is_active) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10)",
            ban.TargetSteamId, ban.TargetName, ban.TargetIp, ban.AdminSteamId, ban.AdminName,
            ban.Reason, ban.CreatedAt, ban.ExpiresAt, ban.Duration, ban.IsActive);

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool BanRepository::Update(const Ban& ban)
{
    try
    {
        Database::Instance().ExecutePrepared(
            "update_ban",
            "UPDATE bans SET target_name = $2, is_active = $3, removed_at = $4, "
            "removed_by = $5, removed_reason = $6 WHERE id = $1",
            ban.Id, ban.TargetName, ban.IsActive, ban.RemovedAt, ban.RemovedBy,
            ban.RemovedReason);

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool BanRepository::Remove(int64_t banId, int64_t removedBy, const std::string& reason)
{
    try
    {
        Database::Instance().ExecutePrepared(
            "remove_ban",
            "UPDATE bans SET is_active = false, removed_at = $2, removed_by = $3, removed_reason = $4 WHERE id = $1",
            banId, Utils::TimeUtils::Now(), removedBy, reason);

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

int BanRepository::ExpireOldBans()
{
    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "expire_old_bans",
            "UPDATE bans SET is_active = false WHERE is_active = true AND expires_at > 0 AND expires_at <= $1",
            Utils::TimeUtils::Now());

        return result.affected_rows();
    }
    catch (const std::exception& e)
    {
        return 0;
    }
}

std::vector<Ban> BanRepository::GetHistory(int64_t steamId)
{
    std::vector<Ban> bans;

    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "get_ban_history",
            "SELECT * FROM bans WHERE target_steam_id = $1 ORDER BY created_at DESC",
            steamId);

        for (const auto& row : result)
        {
            bans.push_back(ParseRow(row));
        }
    }
    catch (const std::exception& e)
    {
        // Log error
    }

    return bans;
}

Ban BanRepository::ParseRow(const pqxx::row& row)
{
    Ban ban;
    ban.Id = row["id"].as<int64_t>();
    ban.TargetSteamId = row["target_steam_id"].as<int64_t>();
    ban.TargetName = row["target_name"].c_str();
    ban.TargetIp = row["target_ip"].c_str();
    ban.AdminSteamId = row["admin_steam_id"].as<int64_t>();
    ban.AdminName = row["admin_name"].c_str();
    ban.Reason = row["reason"].c_str();
    ban.CreatedAt = row["created_at"].as<int64_t>();
    ban.ExpiresAt = row["expires_at"].as<int64_t>();
    ban.Duration = row["duration"].as<int64_t>();
    ban.IsActive = row["is_active"].as<bool>();

    if (!row["removed_at"].is_null())
    {
        ban.RemovedAt = row["removed_at"].as<int64_t>();
        ban.RemovedBy = row["removed_by"].as<int64_t>();
        ban.RemovedReason = row["removed_reason"].c_str();
    }

    return ban;
}

} // namespace AdminSystem::Database
