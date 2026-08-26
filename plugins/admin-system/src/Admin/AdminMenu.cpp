#include "AdminMenu.hpp"

#include "../Core/App.hpp"
#include "../Core/Plugin.hpp"
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

using VoltMod::Core::Translations;
using VoltMod::Menu::MenuBuilder;
using VoltMod::Players::PlayerManager;

std::shared_ptr<VoltMod::MenuView> BuildAdminMainMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;
    auto& adminMgr = app.Admins;
    auto& access = app.Access;
    auto& plrMgr = app.Runtime.Players;

    auto* adminPlayer = plrMgr.GetPlayerBySlot(adminSlot);
    if (!adminPlayer)
        return nullptr;

    int64_t adminSid = adminPlayer->GetSteamID();

    // Version rendered small and gray next to the gold panel title, matching the pager style.
    auto title = std::format("{} <font class='fontSize-s' color='#887755'>v{}</font>", tr.Get("panel.admin", adminSlot),
                             app.Version);

    return MenuBuilder(title)
        .AddSubmenu(
            tr.Get("category.punish", adminSlot),
            [&app, adminSlot](int) { return Menu::BuildPunishMenu(app, adminSlot); },
            access.HasAnyPermission(adminSid, "cdoe"))
        .AddSubmenu(
            tr.Get("category.control", adminSlot),
            [&app, adminSlot](int) { return Menu::BuildControlMenu(app, adminSlot); },
            access.HasAnyPermission(adminSid, "bskz"))
        .AddSubmenu(
            tr.Get("category.effects", adminSlot),
            [&app, adminSlot](int) { return Menu::BuildEffectsMenu(app, adminSlot); },
            access.HasAnyPermission(adminSid, "fjz"))
        .AddSubmenu(
            tr.Get("category.fun", adminSlot), [&app, adminSlot](int) { return Menu::BuildFunMenu(app, adminSlot); },
            access.HasAnyPermission(adminSid, "gz"))
        .AddSubmenu(
            tr.Get("category.map", adminSlot), [&app, adminSlot](int) { return Menu::BuildMapMenu(app, adminSlot); },
            access.HasAnyPermission(adminSid, "mvz"))
        .AddSubmenu(
            tr.Get("category.chatSettings", adminSlot),
            [&app, adminSlot](int) { return Menu::BuildChatSettingsMenu(app, adminSlot); }, adminMgr.IsAdmin(adminSid))
        .Build();
}

}  // namespace AdminSystem::Admin
