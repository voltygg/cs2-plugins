#include "Admin/Menu/RootMenu.hpp"

#include "Admin/AdminManager.hpp"
#include "Admin/Menu/MenuContext.hpp"
#include "Admin/Menu/Tabs/MapVoteTab.hpp"
#include "Admin/Menu/Tabs/MySettingsTab.hpp"
#include "Admin/Menu/Tabs/PlayerActionsTab.hpp"
#include "Admin/Menu/Tabs/PlayerFunTab.hpp"
#include "Admin/Menu/Tabs/PunishTab.hpp"
#include "Admin/Menu/Tabs/RoundModesTab.hpp"
#include "Core/App.hpp"
#include "Core/Permissions.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <algorithm>
#include <array>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace AdminSystem::Admin::Menu
{

using VoltMod::MenuBuilder;
using VoltMod::SubmenuRow;

/** One category of the root menu, in the order the Panorama surface shows them as tabs. */
struct Category
{
    std::string_view LabelKey;
    std::shared_ptr<VoltMod::Menu> (*Build)(const MenuContext& ctx);
    /** Any one of these opens the category; empty means any admin. */
    std::vector<std::string_view> Permissions;
    /** The tab icon, one of the names in panorama/screens/admin_menu/icons.j2. */
    std::string_view Icon;
};

static const std::array<Category, 6> Categories{{
    {"category.punish", &BuildPunishTab, {Permission::Kick, Permission::Ban, Permission::Mute, Permission::Unban}, "punish"},
    {"category.playerActions", &BuildPlayerActionsTab, {Permission::Hide, Permission::Control, Permission::Weapon}, "control"},
    {"category.playerFun", &BuildPlayerFunTab, {Permission::Fun, Permission::Bhop}, "effects"},
    {"category.roundModes", &BuildRoundModesTab, {Permission::FunMode}, "fun"},
    {"category.mapVote", &BuildMapVoteTab, {Permission::Map, Permission::Vote}, "map"},
    {"category.mySettings", &BuildMySettingsTab, {}, "chat"},
}};

std::shared_ptr<VoltMod::Menu> BuildRootMenu(AdminSystem::App& app, int adminSlot)
{
    const std::optional<MenuContext> ctx = MenuContext::For(app, adminSlot);
    if (!ctx)
        return nullptr;

    // The version goes in the subtitle rather than into the title as markup: both menu hosts
    // show a subtitle, and only one of them can render a <font> tag.
    MenuBuilder builder(ctx->Translate("panel.admin"));
    builder.Subtitle(std::format("v{}", app.Runtime.Version));

    for (const Category& category : Categories)
    {
        const bool allowed = category.Permissions.empty()
                                 ? app.Admins.IsAdmin(ctx->Admin.SteamId)
                                 : std::ranges::any_of(category.Permissions, [&](std::string_view permission) {
                                       return app.Access.HasPermission(ctx->Admin.SteamId, permission);
                                   });
        builder.Add(SubmenuRow{.Label = ctx->Translate(category.LabelKey),
                               .Build = [ctx = *ctx, build = category.Build](int) { return build(ctx); },
                               .Enabled = allowed,
                               .Icon = std::string(category.Icon)});
    }

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
