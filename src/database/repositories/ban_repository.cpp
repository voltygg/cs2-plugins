#include "ban_repository.h"
#include "../database.h"
#include "../../utils/time.h"

namespace database {

std::optional<Ban> BanRepository::FindActiveBySteamID(int64_t steam_id)
{
    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "find_active_ban_by_steamid",
            "SELECT * FROM bans WHERE target_steam_id = $1 AND is_active = true AND (expires_at = 0 OR expires_at > $2)",
            steam_id,
            utils::Time::Now()
        );

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

std::optional<Ban> BanRepository::FindActiveByIP(const std::string& ip_address)
{
    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "find_active_ban_by_ip",
            "SELECT * FROM bans WHERE target_ip = $1 AND is_active = true AND (expires_at = 0 OR expires_at > $2)",
            ip_address,
            utils::Time::Now()
        );

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
            utils::Time::Now()
        );

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
            "INSERT INTO bans (target_steam_id, target_name, target_ip, admin_steam_id, admin_name, reason, created_at, expires_at, duration, is_active) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10)",
            ban.target_steam_id,
            ban.target_name,
            ban.target_ip,
            ban.admin_steam_id,
            ban.admin_name,
            ban.reason,
            ban.created_at,
            ban.expires_at,
            ban.duration,
            ban.is_active
        );

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
            "UPDATE bans SET target_name = $2, is_active = $3, removed_at = $4, removed_by = $5, removed_reason = $6 WHERE id = $1",
            ban.id,
            ban.target_name,
            ban.is_active,
            ban.removed_at,
            ban.removed_by,
            ban.removed_reason
        );

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool BanRepository::Remove(int64_t id, int64_t removed_by, const std::string& reason)
{
    try
    {
        Database::Instance().ExecutePrepared(
            "remove_ban",
            "UPDATE bans SET is_active = false, removed_at = $2, removed_by = $3, removed_reason = $4 WHERE id = $1",
            id,
            utils::Time::Now(),
            removed_by,
            reason
        );

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
            utils::Time::Now()
        );

        return result.affected_rows();
    }
    catch (const std::exception& e)
    {
        return 0;
    }
}

std::vector<Ban> BanRepository::GetHistory(int64_t steam_id, int limit)
{
    std::vector<Ban> bans;

    try
    {
        auto result = Database::Instance().ExecutePrepared(
            "get_ban_history",
            "SELECT * FROM bans WHERE target_steam_id = $1 ORDER BY created_at DESC LIMIT $2",
            steam_id,
            limit
        );

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
    ban.id = row["id"].as<int64_t>();
    ban.target_steam_id = row["target_steam_id"].as<int64_t>();
    ban.target_name = row["target_name"].c_str();
    ban.target_ip = row["target_ip"].c_str();
    ban.admin_steam_id = row["admin_steam_id"].as<int64_t>();
    ban.admin_name = row["admin_name"].c_str();
    ban.reason = row["reason"].c_str();
    ban.created_at = row["created_at"].as<int64_t>();
    ban.expires_at = row["expires_at"].as<int64_t>();
    ban.duration = row["duration"].as<int64_t>();
    ban.is_active = row["is_active"].as<bool>();

    if (!row["removed_at"].is_null())
    {
        ban.removed_at = row["removed_at"].as<int64_t>();
        ban.removed_by = row["removed_by"].as<int64_t>();
        ban.removed_reason = row["removed_reason"].c_str();
    }

    return ban;
}

} // namespace database
