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
#include <format>

namespace AdminSystem::Admin
{

using VoltMod::MenuBuilder;
using VoltMod::SubmenuRow;

std::shared_ptr<VoltMod::Menu> BuildAdminMainMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;
    auto& adminMgr = app.Admins;
    auto& access = app.Access;
    auto& plrMgr = app.Runtime.Players;

    auto* adminPlayer = plrMgr.Get(adminSlot);
    if (!adminPlayer)
        return nullptr;

    int64_t adminSid = adminPlayer->SteamId();

    // The version goes in the subtitle rather than into the title as markup: both menu hosts
    // show a subtitle, and only one of them can render a <font> tag.
    return MenuBuilder(tr.Get("panel.admin", adminSlot))
        .Subtitle(std::format("v{}", app.Version))
        .Add(SubmenuRow{.Label = tr.Get("category.punish", adminSlot),
                        .Build = [&app, adminSlot](int) { return Menu::BuildPunishMenu(app, adminSlot); },
                        .Enabled = access.HasAnyPermission(adminSid, "cdoe")})
        .Add(SubmenuRow{.Label = tr.Get("category.control", adminSlot),
                        .Build = [&app, adminSlot](int) { return Menu::BuildControlMenu(app, adminSlot); },
                        .Enabled = access.HasAnyPermission(adminSid, "bskz")})
        .Add(SubmenuRow{.Label = tr.Get("category.effects", adminSlot),
                        .Build = [&app, adminSlot](int) { return Menu::BuildEffectsMenu(app, adminSlot); },
                        .Enabled = access.HasAnyPermission(adminSid, "fjz")})
        .Add(SubmenuRow{.Label = tr.Get("category.fun", adminSlot),
                        .Build = [&app, adminSlot](int) { return Menu::BuildFunMenu(app, adminSlot); },
                        .Enabled = access.HasAnyPermission(adminSid, "gz")})
        .Add(SubmenuRow{.Label = tr.Get("category.map", adminSlot),
                        .Build = [&app, adminSlot](int) { return Menu::BuildMapMenu(app, adminSlot); },
                        .Enabled = access.HasAnyPermission(adminSid, "mvz")})
        .Add(SubmenuRow{.Label = tr.Get("category.chatSettings", adminSlot),
                        .Build = [&app, adminSlot](int) { return Menu::BuildChatSettingsMenu(app, adminSlot); },
                        .Enabled = adminMgr.IsAdmin(adminSid)})
        .Build();
}

}  // namespace AdminSystem::Admin
