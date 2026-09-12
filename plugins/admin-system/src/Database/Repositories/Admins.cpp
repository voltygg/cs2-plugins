#include "Admins.hpp"

#include "../JsonList.hpp"
#include "../Tables/Schema.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Time.hpp>
#include <string_view>
#include <utility>

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
    return _db.RunOr("find_all_admins", [](auto& conn) {
        const Tables::Admins t;
        std::vector<Admin> admins;
        for (const auto& row : conn(sqlpp::select(sqlpp::all_of(t)).from(t)))
        {
            admins.push_back(
                Admin{.Id = row.id,
                      .SteamId = row.steamId,
                      .Name = std::string(row.name),
                      .Groups = ReadGroupList(row.groups, "admins.groups", std::to_string(row.steamId)),
                      .Flags = std::string(row.flags),
                      .Immunity = static_cast<int32_t>(row.immunity),
                      .DisplayPrefix = row.displayPrefix,
                      .NameColor = std::string(row.nameColor),
                      .MessageColor = std::string(row.messageColor),
                      .Language = std::string(row.language),
                      .CreatedAt = row.createdAt,
                      .UpdatedAt = row.updatedAt});
        }
        return admins;
    });
}

std::vector<AdminGroup> AdminRepository::FindAllGroups()
{
    return _db.RunOr("find_all_admin_groups", [](auto& conn) {
        const Tables::AdminGroups t;
        std::vector<AdminGroup> groups;
        for (const auto& row : conn(sqlpp::select(sqlpp::all_of(t)).from(t)))
        {
            groups.push_back(
                AdminGroup{.Id = row.id,
                           .Name = std::string(row.name),
                           .Flags = std::string(row.flags),
                           .Immunity = static_cast<int32_t>(row.immunity),
                           .Inherits = ReadGroupList(row.inherits, "admin_groups.inherits", row.name),
                           .ChatPrefix = std::string(row.chatPrefix),
                           .PrefixColor = std::string(row.prefixColor),
                           .NameColor = std::string(row.nameColor),
                           .MessageColor = std::string(row.messageColor),
                           .CreatedAt = row.createdAt,
                           .UpdatedAt = row.updatedAt});
        }
        return groups;
    });
}

std::unordered_map<int64_t, std::vector<std::string>> AdminRepository::FindGroupsForServer(const std::string& serverTag)
{
    return _db.RunOr("find_server_groups", [serverTag](auto& conn) {
        const Tables::AdminServerGroups t;
        std::unordered_map<int64_t, std::vector<std::string>> grants;
        for (const auto& row : conn(sqlpp::select(t.adminSteamId, t.groupName).from(t).where(t.serverTag == serverTag)))
            grants[row.adminSteamId].emplace_back(row.groupName);
        return grants;
    });
}

void AdminRepository::UpdateChatStyleAsync(int64_t steamId, bool displayPrefix, const std::string& nameColor,
                                           const std::string& messageColor)
{
    _db.RunAsync("update_admin_chat_style",
                 [steamId, displayPrefix, nameColor, messageColor, now = Time::Now()](auto& conn) {
                     const Tables::Admins t;
                     conn(sqlpp::update(t)
                              .set(t.displayPrefix = displayPrefix, t.nameColor = nameColor,
                                   t.messageColor = messageColor, t.updatedAt = now)
                              .where(t.steamId == steamId));
                 });
}

void AdminRepository::UpdateLanguageAsync(int64_t steamId, const std::string& lang)
{
    _db.RunAsync("update_admin_language", [steamId, lang, now = Time::Now()](auto& conn) {
        const Tables::Admins t;
        conn(sqlpp::update(t).set(t.language = lang, t.updatedAt = now).where(t.steamId == steamId));
    });
}

void AdminRepository::SetFrozenAsync(int64_t steamId, int64_t frozenBy, const std::string& reason)
{
    _db.RunAsync("set_admin_frozen", [steamId, frozenBy, reason, now = Time::Now()](auto& conn) {
        const Tables::Admins t;
        conn(sqlpp::update(t)
                 .set(t.isFrozen = true, t.frozenAt = now, t.frozenBy = frozenBy, t.freezeReason = reason,
                      t.updatedAt = now)
                 .where(t.steamId == steamId));
    });
}

void AdminRepository::ClearFrozenAsync(int64_t steamId)
{
    _db.RunAsync("clear_admin_frozen", [steamId, now = Time::Now()](auto& conn) {
        const Tables::Admins t;
        conn(sqlpp::update(t)
                 .set(t.isFrozen = false, t.frozenAt = 0, t.frozenBy = 0, t.freezeReason = "", t.updatedAt = now)
                 .where(t.steamId == steamId));
    });
}

void AdminRepository::FindFrozenAsync(std::function<void(std::vector<FrozenAdmin>)> onDone)
{
    _db.RunAsync(
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
        // On failure the callback never fires and the caller keeps its cached set.
        std::move(onDone));
}

}  // namespace AdminSystem::Database
