#include "AdminMenu.hpp"
#include "../Core/Managers.hpp"
#include <CS2Kit/Core/Services.hpp>

#include "AdminManager.hpp"
#include "Menu/AdminMenu_ChatSettings.hpp"
#include "Menu/AdminMenu_Control.hpp"
#include "Menu/AdminMenu_Effects.hpp"
#include "Menu/AdminMenu_Punish.hpp"

#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/Translations.hpp>

using CS2Kit::Core::Kit;

namespace AdminSystem::Admin
{

using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Players::PlayerManager;
using CS2Kit::Utils::Translations;

std::shared_ptr<::CS2Kit::Menu::Menu> BuildAdminMainMenu(int adminSlot)
{
    auto& tr = Kit().Translations;
    auto& adminMgr = Sys().Admins;
    auto& plrMgr = Kit().Players;

    auto* adminPlayer = plrMgr.GetPlayerBySlot(adminSlot);
    if (!adminPlayer)
        return nullptr;

    int64_t adminSid = adminPlayer->GetSteamID();

    return MenuBuilder(tr.Get("panel.admin"))
        .AddSubmenu(
            tr.Get("category.punish"), [adminSlot](int) { return Menu::BuildPunishMenu(adminSlot); },
            adminMgr.HasAnyPermission(adminSid, "cdopq"))
        .AddSubmenu(
            tr.Get("category.control"), [adminSlot](int) { return Menu::BuildControlMenu(adminSlot); },
            adminMgr.HasAnyPermission(adminSid, "bskz"))
        .AddSubmenu(
            tr.Get("category.effects"), [adminSlot](int) { return Menu::BuildEffectsMenu(adminSlot); },
            adminMgr.HasAnyPermission(adminSid, "fz"))
        .AddSubmenu(
            tr.Get("category.chatSettings"), [adminSlot](int) { return Menu::BuildChatSettingsMenu(adminSlot); },
            adminMgr.IsAdmin(adminSid))
        .Build();
}

}  // namespace AdminSystem::Admin
