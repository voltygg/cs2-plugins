#include "AdminMenu.hpp"

#include "../Core/App.hpp"
#include "../Plugin.hpp"
#include "AdminManager.hpp"
#include "Menu/AdminMenu_ChatSettings.hpp"
#include "Menu/AdminMenu_Control.hpp"
#include "Menu/AdminMenu_Effects.hpp"
#include "Menu/AdminMenu_Fun.hpp"
#include "Menu/AdminMenu_Map.hpp"
#include "Menu/AdminMenu_Punish.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <array>
#include <format>
#include <memory>
#include <string>
#include <string_view>

namespace AdminSystem::Admin
{

using VoltMod::MenuBuilder;
using VoltMod::SubmenuRow;

/** One category of the root menu, in the order the Panorama surface shows them as tabs. */
struct Category
{
    std::string_view LabelKey;
    std::shared_ptr<VoltMod::Menu> (*Build)(AdminSystem::App& app, int adminSlot);
    /** Permission letters, any one of which opens the category; empty means any admin. */
    std::string_view Flags;
};

static constexpr std::array<Category, 6> Categories{{
    {"category.punish", &Menu::BuildPunishMenu, "cdoe"},
    {"category.control", &Menu::BuildControlMenu, "bskz"},
    {"category.effects", &Menu::BuildEffectsMenu, "fjz"},
    {"category.fun", &Menu::BuildFunMenu, "gz"},
    {"category.map", &Menu::BuildMapMenu, "mvz"},
    {"category.chatSettings", &Menu::BuildChatSettingsMenu, ""},
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
    builder.Subtitle(std::format("v{}", app.Version));

    for (const Category& category : Categories)
    {
        const bool allowed = category.Flags.empty() ? app.Admins.IsAdmin(adminSteamId)
                                                    : app.Access.HasAnyPermission(adminSteamId, std::string(category.Flags));
        builder.Add(SubmenuRow{.Label = translations.Get(std::string(category.LabelKey), adminSlot),
                               .Build = [&app, adminSlot, build = category.Build](int) { return build(app, adminSlot); },
                               .Enabled = allowed});
    }

    return builder.Build();
}

}  // namespace AdminSystem::Admin
