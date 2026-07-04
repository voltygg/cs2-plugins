#include "AdminMenu_Unban.hpp"

#include "../../Core/ChatService.hpp"
#include "../../Core/Managers.hpp"
#include "../../Core/Permissions.hpp"
#include "../AdminManager.hpp"
#include "MenuHelpers.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Menu/MenuPresets.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>
#include <string>
#include <utility>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Menu
{

using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Utils::StringUtils;
using CS2Kit::Utils::TimeUtils;

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

void ConfirmAndUnban(int adminSlot, int64_t banId, const std::string& targetName)
{
    auto& tr = Engine().Translations;

    auto* admin = Engine().Players.GetPlayerBySlot(adminSlot);
    if (!admin)
        return;

    // The Unban flag may have been revoked (e.g. !admin_reload) while the menu was open.
    if (!App().Admins.HasPermission(admin->GetSteamID(), Permission::Unban))
    {
        App().Chat.Reply(adminSlot, tr.Get("punish.notAllowed", adminSlot));
        Engine().Menus.CloseAllMenus(adminSlot);
        return;
    }

    // RemoveBan broadcasts "unbanned"; the extra reply covers broadcasts being disabled and
    // returns false when another server already lifted the ban.
    if (App().Punishments.RemoveBan(banId, admin->GetSteamID(), tr.Get("reason.unbannedByAdmin")))
        App().Chat.Reply(adminSlot, tr.Get("unban.done", adminSlot, {{"name", targetName}}));
    else
        App().Chat.Reply(adminSlot, tr.Get("unban.gone", adminSlot));

    Engine().Menus.CloseAllMenus(adminSlot);
}

std::shared_ptr<CS2Kit::MenuView> BuildUnbanConfirm(int adminSlot, const BanRow& row)
{
    auto& tr = Engine().Translations;

    ::CS2Kit::Menu::ConfirmDialogSpec spec{
        .Title = std::format("{}: {}", tr.Get("punish.confirmTitle", adminSlot), tr.Get("action.unban", adminSlot)),
        .ConfirmLabel = tr.Get("punish.confirm", adminSlot),
        .CancelLabel = tr.Get("punish.cancel", adminSlot),
        .OnConfirm = [banId = row.Id, name = row.Name](int slot) { ConfirmAndUnban(slot, banId, name); },
    };
    spec.BodyLines.push_back(std::format("{}: {}", tr.Get("punish.target", adminSlot), row.Name));
    spec.BodyLines.push_back(
        std::format("{}: {}", tr.Get("punish.duration", adminSlot), ExpiryLabel(row.ExpiresAt, adminSlot)));
    spec.BodyLines.push_back(
        std::format("{}: {}", tr.Get("punish.reason", adminSlot), StringUtils::TruncateUtf8(row.Reason, 40)));

    return ::CS2Kit::Menu::BuildConfirmDialog(std::move(spec));
}

}  // namespace

std::shared_ptr<CS2Kit::MenuView> BuildUnbanMenu(int adminSlot)
{
    auto& tr = Engine().Translations;

    MenuBuilder builder(tr.Get("unban.title", adminSlot));

    const auto bans = App().Punishments.GetActiveBans();
    for (const auto& ban : bans)
    {
        BanRow row{.Id = ban.Id,
                   .Name = MenuDisplayName(ban.TargetSteamId, ban.TargetName),
                   .ExpiresAt = ban.ExpiresAt,
                   .Reason = ban.Reason};
        auto label = std::format("{} — {}", row.Name, ExpiryLabel(row.ExpiresAt, adminSlot));

        builder.AddButton(
            label, [row = std::move(row)](int slot) { Engine().Menus.OpenMenu(slot, BuildUnbanConfirm(slot, row)); });
    }

    // Never show a dead-end empty page.
    if (bans.empty())
        builder.AddButton(tr.Get("unban.noBans", adminSlot), [](int) {}, false);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
