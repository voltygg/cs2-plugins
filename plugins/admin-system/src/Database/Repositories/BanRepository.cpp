#include "BanRepository.hpp"
#include "../../Core/Managers.hpp"

#include "../Database.hpp"

#include <CS2Kit/Database/DbResult.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>

namespace AdminSystem::Database
{

using namespace CS2Kit::Utils;
using CS2Kit::Database::TryOr;

std::optional<Ban> BanRepository::FindActiveBySteamId(int64_t steamId)
{
    return TryOr<std::optional<Ban>>(std::nullopt, "BanRepository::FindActiveBySteamId", [&]() -> std::optional<Ban> {
        auto result = App().Db.ExecutePrepared("find_active_ban_by_steamid",
                                               "SELECT * FROM bans WHERE target_steam_id = $1 AND "
                                               "is_active = true AND (expires_at = 0 OR expires_at > $2)",
                                               steamId, TimeUtils::Now());
        if (result.empty())
            return std::nullopt;
        return ParseRow(result[0]);
    });
}

std::optional<Ban> BanRepository::FindActiveByIp(const std::string& ip)
{
    return TryOr<std::optional<Ban>>(std::nullopt, "BanRepository::FindActiveByIp", [&]() -> std::optional<Ban> {
        auto result = App().Db.ExecutePrepared("find_active_ban_by_ip",
                                               "SELECT * FROM bans WHERE target_ip = $1 AND "
                                               "is_active = true AND (expires_at = 0 OR expires_at > $2)",
                                               ip, TimeUtils::Now());
        if (result.empty())
            return std::nullopt;
        return ParseRow(result[0]);
    });
}

std::vector<Ban> BanRepository::FindAllActive()
{
    return TryOr(std::vector<Ban>{}, "BanRepository::FindAllActive", [&] {
        std::vector<Ban> bans;
        auto result = App().Db.ExecutePrepared(
            "find_all_active_bans", "SELECT * FROM bans WHERE is_active = true AND (expires_at = 0 OR expires_at > $1)",
            TimeUtils::Now());
        for (const auto& row : result)
            bans.push_back(ParseRow(row));
        return bans;
    });
}

bool BanRepository::Create(Ban& ban)
{
    return TryOr(false, "BanRepository::Create", [&] {
        auto result = App().Db.ExecutePrepared(
            "create_ban",
            "INSERT INTO bans (target_steam_id, target_name, target_ip, admin_steam_id, admin_name, reason, "
            "created_at, expires_at, duration, is_active) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10) RETURNING id",
            ban.TargetSteamId, ban.TargetName, ban.TargetIp, ban.AdminSteamId, ban.AdminName, ban.Reason, ban.CreatedAt,
            ban.ExpiresAt, ban.Duration, ban.IsActive);
        if (!result.empty())
            ban.Id = result[0]["id"].as<int64_t>();
        return true;
    });
}

bool BanRepository::Update(const Ban& ban)
{
    return TryOr(false, "BanRepository::Update", [&] {
        App().Db.ExecutePrepared("update_ban",
                                 "UPDATE bans SET target_name = $2, is_active = $3, removed_at = $4, "
                                 "removed_by = $5, removed_reason = $6 WHERE id = $1",
                                 ban.Id, ban.TargetName, ban.IsActive, ban.RemovedAt, ban.RemovedBy, ban.RemovedReason);
        return true;
    });
}

bool BanRepository::Remove(int64_t banId, int64_t removedBy, const std::string& reason)
{
    return TryOr(false, "BanRepository::Remove", [&] {
        App().Db.ExecutePrepared(
            "remove_ban",
            "UPDATE bans SET is_active = false, removed_at = $2, removed_by = $3, removed_reason = $4 WHERE id = $1",
            banId, TimeUtils::Now(), removedBy, reason);
        return true;
    });
}

int BanRepository::ExpireOldBans()
{
    return TryOr(0, "BanRepository::ExpireOldBans", [&]() -> int {
        auto result = App().Db.ExecutePrepared(
            "expire_old_bans",
            "UPDATE bans SET is_active = false WHERE is_active = true AND expires_at > 0 AND expires_at <= $1",
            TimeUtils::Now());
        return result.affected_rows();
    });
}

std::vector<Ban> BanRepository::GetHistory(int64_t steamId)
{
    return TryOr(std::vector<Ban>{}, "BanRepository::GetHistory", [&] {
        std::vector<Ban> bans;
        auto result = App().Db.ExecutePrepared(
            "get_ban_history", "SELECT * FROM bans WHERE target_steam_id = $1 ORDER BY created_at DESC", steamId);
        for (const auto& row : result)
            bans.push_back(ParseRow(row));
        return bans;
    });
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

}  // namespace AdminSystem::Database
