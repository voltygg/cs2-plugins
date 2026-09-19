#include "Admin/AdminMenu.hpp"

#include "Admin/AdminManager.hpp"
#include "Admin/Menu/AdminMenu_ChatSettings.hpp"
#include "Admin/Menu/AdminMenu_Control.hpp"
#include "Admin/Menu/AdminMenu_Effects.hpp"
#include "Admin/Menu/AdminMenu_Fun.hpp"
#include "Admin/Menu/AdminMenu_Map.hpp"
#include "Admin/Menu/AdminMenu_Punish.hpp"
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
#include <string>
#include <string_view>
#include <vector>

namespace AdminSystem::Admin
{

using VoltMod::MenuBuilder;
using VoltMod::SubmenuRow;

/** One category of the root menu, in the order the Panorama surface shows them as tabs. */
struct Category
{
    std::string_view LabelKey;
    std::shared_ptr<VoltMod::Menu> (*Build)(AdminSystem::App& app, int adminSlot);
    /** Any one of these opens the category; empty means any admin. */
    std::vector<std::string_view> Permissions;
    /** The tab icon, one of the names in panorama/screens/admin_menu/icons.j2. */
    std::string_view Icon;
};

static const std::array<Category, 6> Categories{{
    {"category.punish", &Menu::BuildPunishMenu, {Permission::Kick, Permission::Ban, Permission::Mute, Permission::Unban}, "punish"},
    {"category.control", &Menu::BuildControlMenu, {Permission::Hide, Permission::Control, Permission::Weapon}, "control"},
    {"category.effects", &Menu::BuildEffectsMenu, {Permission::Fun, Permission::Bhop}, "effects"},
    {"category.fun", &Menu::BuildFunMenu, {Permission::FunMode}, "fun"},
    {"category.map", &Menu::BuildMapMenu, {Permission::Map, Permission::Vote}, "map"},
    {"category.chatSettings", &Menu::BuildChatSettingsMenu, {}, "chat"},
}};

std::shared_ptr<VoltMod::Menu> BuildAdminMainMenu(AdminSystem::App& app, int adminSlot)
{
    auto& translations = app.Runtime.Translations;
    auto* adminPlayer = app.Runtime.Players.Get(adminSlot);
    if (!adminPlayer)
        return nullptr;

    const int64_t adminSteamId = adminPlayer->SteamId();

    // The version goes in the subtitle rather than into the title as markup: both menu hosts
    // show a subtitle, and only one of them can render a <font> tag.
    MenuBuilder builder(translations.Get("panel.admin", adminSlot));
    builder.Subtitle(std::format("v{}", app.Runtime.Version));

    for (const Category& category : Categories)
    {
        const bool allowed = category.Permissions.empty()
                                 ? app.Admins.IsAdmin(adminSteamId)
                                 : std::ranges::any_of(category.Permissions, [&](std::string_view permission) {
                                       return app.Access.HasPermission(adminSteamId, permission);
                                   });
        builder.Add(
            SubmenuRow{.Label = translations.Get(category.LabelKey, adminSlot),
                       .Build = [&app, adminSlot, build = category.Build](int) { return build(app, adminSlot); },
                       .Enabled = allowed,
                       .Icon = std::string(category.Icon)});
    }

    return builder.Build();
}

}  // namespace AdminSystem::Admin
