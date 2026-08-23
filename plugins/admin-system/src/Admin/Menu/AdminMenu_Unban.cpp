#include "AdminMenu_Unban.hpp"

#include "../../Core/App.hpp"
#include "../../Core/ChatService.hpp"
#include "../../Core/Permissions.hpp"
#include "../AdminManager.hpp"
#include "Labels.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/StringUtils.hpp>
#include <CS2Kit/Core/Translations.hpp>
#include <CS2Kit/Menu/Flow.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Runtime.hpp>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace AdminSystem::Admin::Menu
{

using CS2Kit::Core::StringUtils;
using CS2Kit::Menu::MenuBuilder;

namespace
{

/** The slice of a Ban the menu needs; each row's lambda holds a copy, so keep it small. */
struct BanRow
{
    int64_t Id = 0;
    std::string Name;
    int64_t ExpiresAt = 0;
    std::string Reason;
};

void StartUnbanConfirm(App& app, int adminSlot, BanRow row)
{
    CS2Kit::Flow<BanRow>::Create(std::move(row))
        // The Unban flag may have been revoked (e.g. !admin_reload) while the menu was open.
        ->OnValidate([&app](int slot, const BanRow&) -> std::optional<std::string> {
            auto* admin = app.Runtime.Players.GetPlayerBySlot(slot);
            if (!admin || !app.Admins.HasPermission(admin->GetSteamID(), Permission::Unban))
                return "punish.notAllowed";
            return std::nullopt;
        })
        ->WithConfirm(
            [&app](int slot) {
                auto& tr = app.Runtime.Translations;
                return std::format("{}: {}", tr.Get("punish.confirmTitle", slot), tr.Get("action.unban", slot));
            },
            [&app](int slot, const BanRow& r) {
                auto& tr = app.Runtime.Translations;
                std::vector<std::pair<std::string, std::string>> rows;
                rows.emplace_back(tr.Get("punish.target", slot), r.Name);
                rows.emplace_back(tr.Get("punish.duration", slot), ExpiryLabel(tr, r.ExpiresAt, slot));
                rows.emplace_back(tr.Get("punish.reason", slot), StringUtils::TruncateUtf8(r.Reason, 40));
                return rows;
            },
            [&app](int slot) { return app.Runtime.Translations.Get("punish.confirm", slot); },
            [&app](int slot) { return app.Runtime.Translations.Get("punish.cancel", slot); })
        ->OnFinish([&app](int slot, BanRow& r) {
            auto& tr = app.Runtime.Translations;
            auto* admin = app.Runtime.Players.GetPlayerBySlot(slot);
            if (!admin)
                return;

            // RemoveBan broadcasts "unbanned"; the extra reply covers broadcasts being disabled and
            // returns false when another server already lifted the ban.
            if (app.Punishments.RemoveBan(r.Id, admin->GetSteamID(), tr.Get("reason.unbannedByAdmin")))
                app.Chat.Reply(slot, tr.Get("unban.done", slot, {{"name", r.Name}}));
            else
                app.Chat.Reply(slot, tr.Get("unban.gone", slot));
        })
        ->Start(adminSlot);
}

}  // namespace

std::shared_ptr<CS2Kit::MenuView> BuildUnbanMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;

    MenuBuilder builder(tr.Get("unban.title", adminSlot));

    const auto bans = app.Punishments.GetActiveBans();
    for (const auto& ban : bans)
    {
        BanRow row{.Id = ban.Id,
                   .Name = StringUtils::DisplayNameOr(ban.TargetSteamId, ban.TargetName),
                   .ExpiresAt = ban.ExpiresAt,
                   .Reason = ban.Reason};
        auto label = std::format("{} - {}", row.Name, ExpiryLabel(tr, row.ExpiresAt, adminSlot));

        builder.AddButton(label, [&app, row = std::move(row)](int slot) { StartUnbanConfirm(app, slot, row); });
    }

    // Never show a dead-end empty page.
    if (bans.empty())
        builder.AddButton(tr.Get("unban.noBans", adminSlot), [](int) {}, false);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
