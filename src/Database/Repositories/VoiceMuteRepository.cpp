#include "VoiceMuteRepository.hpp"
#include "../../Core/Managers.hpp"

#include "../Database.hpp"

#include <CS2Kit/Database/DbResult.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>

namespace AdminSystem::Database
{

using namespace CS2Kit::Utils;
using CS2Kit::Database::TryOr;

std::optional<VoiceMute> VoiceMuteRepository::FindActiveBySteamId(int64_t steamId)
{
    return TryOr<std::optional<VoiceMute>>(std::nullopt, "VoiceMuteRepository::FindActiveBySteamId", [&]() -> std::optional<VoiceMute> {
        auto result = Sys().Db.ExecutePrepared("find_active_voice_mute_by_steamid",
                                                           "SELECT * FROM voice_mutes WHERE target_steam_id = $1 AND "
                                                           "is_active = true AND (expires_at = 0 OR expires_at > $2)",
                                                           steamId, TimeUtils::Now());
        if (result.empty())
            return std::nullopt;
        return ParseRow(result[0]);
    });
}

std::vector<VoiceMute> VoiceMuteRepository::FindAllActive()
{
    return TryOr(std::vector<VoiceMute>{}, "VoiceMuteRepository::FindAllActive", [&] {
        std::vector<VoiceMute> mutes;
        auto result = Sys().Db.ExecutePrepared(
            "find_all_active_voice_mutes",
            "SELECT * FROM voice_mutes WHERE is_active = true AND (expires_at = 0 OR expires_at > $1)",
            TimeUtils::Now());
        for (const auto& row : result)
            mutes.push_back(ParseRow(row));
        return mutes;
    });
}

bool VoiceMuteRepository::Create(VoiceMute& mute)
{
    return TryOr(false, "VoiceMuteRepository::Create", [&] {
        auto result = Sys().Db.ExecutePrepared(
            "create_voice_mute",
            "INSERT INTO voice_mutes (target_steam_id, target_name, admin_steam_id, admin_name, reason, "
            "created_at, expires_at, duration, is_active) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) RETURNING id",
            mute.TargetSteamId, mute.TargetName, mute.AdminSteamId, mute.AdminName, mute.Reason, mute.CreatedAt,
            mute.ExpiresAt, mute.Duration, mute.IsActive);
        if (!result.empty())
            mute.Id = result[0]["id"].as<int64_t>();
        return true;
    });
}

bool VoiceMuteRepository::Remove(int64_t muteId, int64_t removedBy, const std::string& reason)
{
    return TryOr(false, "VoiceMuteRepository::Remove", [&] {
        Sys().Db.ExecutePrepared(
            "remove_voice_mute",
            "UPDATE voice_mutes SET is_active = false, removed_at = $2, removed_by = $3, removed_reason = $4 "
            "WHERE id = $1",
            muteId, TimeUtils::Now(), removedBy, reason);
        return true;
    });
}

int VoiceMuteRepository::ExpireOldVoiceMutes()
{
    return TryOr(0, "VoiceMuteRepository::ExpireOldVoiceMutes", [&]() -> int {
        auto result = Sys().Db.ExecutePrepared(
            "expire_old_voice_mutes",
            "UPDATE voice_mutes SET is_active = false WHERE is_active = true AND expires_at > 0 AND expires_at <= $1",
            TimeUtils::Now());
        return result.affected_rows();
    });
}

std::vector<VoiceMute> VoiceMuteRepository::GetHistory(int64_t steamId)
{
    return TryOr(std::vector<VoiceMute>{}, "VoiceMuteRepository::GetHistory", [&] {
        std::vector<VoiceMute> mutes;
        auto result = Sys().Db.ExecutePrepared(
            "get_voice_mute_history", "SELECT * FROM voice_mutes WHERE target_steam_id = $1 ORDER BY created_at DESC",
            steamId);
        for (const auto& row : result)
            mutes.push_back(ParseRow(row));
        return mutes;
    });
}

VoiceMute VoiceMuteRepository::ParseRow(const pqxx::row& row)
{
    VoiceMute mute;
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
