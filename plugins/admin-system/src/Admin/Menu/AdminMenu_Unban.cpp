#include "AdminMenu_Unban.hpp"

#include "../../Core/App.hpp"
#include "../../Core/ChatService.hpp"
#include "../../Core/Permissions.hpp"
#include "../AdminManager.hpp"
#include "Labels.hpp"
#include "MenuAccess.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Strings.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/Flow.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Runtime.hpp>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace AdminSystem::Admin::Menu
{

using VoltMod::MenuBuilder;
using VoltMod::Strings;

/** The slice of a Ban the menu needs; each row's lambda holds a copy, so keep it small. */
struct BanRow
{
    int64_t Id = 0;
    std::string Name;
    int64_t ExpiresAt = 0;
    std::string Reason;
};

static void StartUnbanConfirm(App& app, int adminSlot, BanRow row)
{
    VoltMod::Flow<BanRow>::Create(app.Runtime.Menus, std::move(row))
        // The Unban flag may have been revoked (e.g. !admin_reload) while the menu was open.
        ->OnValidate([&app](int slot, const BanRow&) -> std::optional<std::string> {
            if (!MayUse(app, slot, Permission::Unban))
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
                rows.emplace_back(tr.Get("punish.reason", slot), Strings::TruncateUtf8(r.Reason, 40));
                return rows;
            },
            [&app](int slot) { return app.Runtime.Translations.Get("punish.confirm", slot); },
            [&app](int slot) { return app.Runtime.Translations.Get("punish.cancel", slot); })
        ->OnFinish([&app](int slot, BanRow& r) {
            auto& tr = app.Runtime.Translations;
            auto* admin = app.Runtime.Players.Get(slot);
            if (!admin)
                return;

            // RemoveBan broadcasts "unbanned"; the extra reply covers broadcasts being disabled and
            // returns false when another server already lifted the ban.
            if (app.Punishments.RemoveBan(r.Id, admin->SteamId(), tr.Get("reason.unbannedByAdmin")))
                app.Chat.Reply(slot, tr.Get("unban.done", slot, {{"name", r.Name}}));
            else
                app.Chat.Reply(slot, tr.Get("unban.gone", slot));
        })
        ->Start(adminSlot);
}

std::shared_ptr<VoltMod::MenuView> BuildUnbanMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;

    MenuBuilder builder(tr.Get("unban.title", adminSlot));

    const auto bans = app.Punishments.GetActiveBans();
    for (const auto& ban : bans)
    {
        BanRow row{.Id = ban.Id,
                   .Name = Strings::DisplayNameOr(ban.TargetSteamId, ban.TargetName),
                   .ExpiresAt = ban.ExpiresAt,
                   .Reason = ban.Reason};
        auto label = std::format("{} - {}", row.Name, ExpiryLabel(tr, row.ExpiresAt, adminSlot));

        builder.Button(label, [&app, row = std::move(row)](int slot) { StartUnbanConfirm(app, slot, row); });
    }

    // Never show a dead-end empty page.
    if (bans.empty())
        builder.Button(tr.Get("unban.noBans", adminSlot), [](int) {}, false);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
