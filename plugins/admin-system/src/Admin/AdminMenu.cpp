#include "AdminMenu.hpp"

#include "../Core/App.hpp"
#include "../Core/Plugin.hpp"
#include "AdminManager.hpp"
#include "Menu/AdminMenu_ChatSettings.hpp"
#include "Menu/AdminMenu_Control.hpp"
#include "Menu/AdminMenu_Effects.hpp"
#include "Menu/AdminMenu_Punish.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Translations.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Runtime.hpp>
#include <format>

namespace AdminSystem::Admin
{

using CS2Kit::Core::Translations;
using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Players::PlayerManager;

std::shared_ptr<CS2Kit::MenuView> BuildAdminMainMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;
    auto& adminMgr = app.Admins;
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
            adminMgr.HasAnyPermission(adminSid, "cdoe"))
        .AddSubmenu(
            tr.Get("category.control", adminSlot),
            [&app, adminSlot](int) { return Menu::BuildControlMenu(app, adminSlot); },
            adminMgr.HasAnyPermission(adminSid, "bsz"))
        .AddSubmenu(
            tr.Get("category.effects", adminSlot),
            [&app, adminSlot](int) { return Menu::BuildEffectsMenu(app, adminSlot); },
            adminMgr.HasAnyPermission(adminSid, "fjz"))
        .AddSubmenu(
            tr.Get("category.chatSettings", adminSlot),
            [&app, adminSlot](int) { return Menu::BuildChatSettingsMenu(app, adminSlot); }, adminMgr.IsAdmin(adminSid))
        .Build();
}

}  // namespace AdminSystem::Admin
