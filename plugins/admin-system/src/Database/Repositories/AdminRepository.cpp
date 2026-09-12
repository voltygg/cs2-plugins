#include "AdminRepository.hpp"

#include "../JsonList.hpp"
#include "../Tables/AdminTables.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Time.hpp>
#include <string_view>
#include <utility>
#include <vector>

namespace AdminSystem::Database
{

namespace Log = VoltMod::Log;
using VoltMod::Time;

/** Malformed list text is a bad row, not a bad database: name it and treat it as empty. */
static std::vector<std::string> ReadGroupList(std::string_view text, std::string_view column, std::string_view row)
{
    auto list = ReadJsonList(text);
    if (!list)
    {
        Log::Warn("Ignoring malformed {} for {}.", column, row);
        return {};
    }
    return std::move(*list);
}

std::vector<Admin> AdminRepository::FindAll()
{
    auto result = _db.RunBlocking("find_all_admins", [](auto& conn) {
        const Tables::Admins t;
        std::vector<Admin> admins;
        for (const auto& row : conn(sqlpp::select(sqlpp::all_of(t)).from(t)))
        {
            Admin admin;
            admin.Id = row.id;
            admin.SteamId = row.steamId;
            admin.Name = row.name;
            admin.Flags = row.flags;
            admin.Immunity = static_cast<int32_t>(row.immunity);
            admin.CreatedAt = row.createdAt;
            admin.UpdatedAt = row.updatedAt;
            admin.Groups = ReadGroupList(row.groups, "admins.groups", std::to_string(row.steamId));
            admin.DisplayPrefix = row.displayPrefix;
            admin.NameColor = row.nameColor;
            admin.MessageColor = row.messageColor;
            admin.Language = row.language;

            admin.BuildFlagBits();
            admins.push_back(std::move(admin));
        }
        return admins;
    });

    if (!result)
        return {};
    return std::move(*result);
}

void AdminRepository::UpdateChatStyle(int64_t steamId, bool displayPrefix, const std::string& nameColor,
                                      const std::string& messageColor)
{
    _db.Run("update_admin_chat_style",
            [steamId, displayPrefix, nameColor, messageColor, now = Time::Now()](auto& conn) {
                const Tables::Admins t;
                conn(sqlpp::update(t)
                         .set(t.displayPrefix = displayPrefix, t.nameColor = nameColor, t.messageColor = messageColor,
                              t.updatedAt = now)
                         .where(t.steamId == steamId));
            });
}

void AdminRepository::UpdateLanguage(int64_t steamId, const std::string& lang)
{
    _db.Run("update_admin_language", [steamId, lang, now = Time::Now()](auto& conn) {
        const Tables::Admins t;
        conn(sqlpp::update(t).set(t.language = lang, t.updatedAt = now).where(t.steamId == steamId));
    });
}

bool AdminRepository::SetFrozen(int64_t steamId, int64_t frozenBy, const std::string& reason)
{
    auto result = _db.RunBlocking("set_admin_frozen", [steamId, frozenBy, reason, now = Time::Now()](auto& conn) {
        const Tables::Admins t;
        conn(sqlpp::update(t)
                 .set(t.isFrozen = true, t.frozenAt = now, t.frozenBy = frozenBy, t.freezeReason = reason,
                      t.updatedAt = now)
                 .where(t.steamId == steamId));
    });
    return result.has_value();
}

bool AdminRepository::ClearFrozen(int64_t steamId)
{
    auto result = _db.RunBlocking("clear_admin_frozen", [steamId, now = Time::Now()](auto& conn) {
        const Tables::Admins t;
        conn(sqlpp::update(t)
                 .set(t.isFrozen = false, t.frozenAt = 0, t.frozenBy = 0, t.freezeReason = "", t.updatedAt = now)
                 .where(t.steamId == steamId));
    });
    return result.has_value();
}

void AdminRepository::FindFrozenAsync(std::function<void(std::vector<FrozenAdmin>)> onDone)
{
    _db.Run(
        "find_frozen_admins",
        [](auto& conn) {
            const Tables::Admins t;
            std::vector<FrozenAdmin> frozen;
            for (const auto& row : conn(sqlpp::select(t.steamId, t.name, t.frozenAt, t.frozenBy, t.freezeReason)
                                            .from(t)
                                            .where(t.isFrozen == true)))
            {
                frozen.push_back({.SteamId = row.steamId,
                                  .Name = std::string(row.name),
                                  .FrozenAt = row.frozenAt,
                                  .FrozenBy = row.frozenBy,
                                  .Reason = std::string(row.freezeReason)});
            }
            return frozen;
        },
        [onDone = std::move(onDone)](VoltMod::DbResult<std::vector<FrozenAdmin>> result) {
            if (!result || !onDone)
                return;  // DB error already logged; keep the caller's cached set.
            onDone(std::move(*result));
        });
}


std::vector<AdminGroup> AdminGroupRepository::FindAll()
{
    auto result = _db.RunBlocking("find_all_admin_groups", [](auto& conn) {
        const Tables::AdminGroups t;
        std::vector<AdminGroup> groups;
        for (const auto& row : conn(sqlpp::select(sqlpp::all_of(t)).from(t)))
        {
            AdminGroup group;
            group.Id = row.id;
            group.Name = row.name;
            group.Flags = row.flags;
            group.Immunity = static_cast<int32_t>(row.immunity);
            group.CreatedAt = row.createdAt;
            group.UpdatedAt = row.updatedAt;
            group.Inherits = ReadGroupList(row.inherits, "admin_groups.inherits", group.Name);
            group.ChatPrefix = row.chatPrefix;
            group.PrefixColor = row.prefixColor;
            group.NameColor = row.nameColor;
            group.MessageColor = row.messageColor;

            group.BuildFlagBits();
            groups.push_back(std::move(group));
        }
        return groups;
    });

    if (!result)
        return {};
    return std::move(*result);
}

}  // namespace AdminSystem::Database
