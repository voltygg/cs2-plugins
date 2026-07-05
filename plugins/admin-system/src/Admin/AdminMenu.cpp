#include "AdminMenu.hpp"

#include "../Core/Managers.hpp"
#include "../Core/Plugin.hpp"
#include "AdminManager.hpp"
#include "Menu/AdminMenu_ChatSettings.hpp"
#include "Menu/AdminMenu_Control.hpp"
#include "Menu/AdminMenu_Effects.hpp"
#include "Menu/AdminMenu_Punish.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin
{

using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Players::PlayerManager;
using CS2Kit::Utils::Translations;

std::shared_ptr<CS2Kit::MenuView> BuildAdminMainMenu(int adminSlot)
{
    auto& tr = Engine().Translations;
    auto& adminMgr = App().Admins;
    auto& plrMgr = Engine().Players;

    auto* adminPlayer = plrMgr.GetPlayerBySlot(adminSlot);
    if (!adminPlayer)
        return nullptr;

    int64_t adminSid = adminPlayer->GetSteamID();

    // Version rendered small and gray next to the gold panel title, matching the pager style.
    auto title = std::format("{} <font class='fontSize-s' color='#887755'>v{}</font>", tr.Get("panel.admin", adminSlot),
                             AdminSystemPlugin::Get().GetVersion());

    return MenuBuilder(title)
        .AddSubmenu(
            tr.Get("category.punish", adminSlot), [adminSlot](int) { return Menu::BuildPunishMenu(adminSlot); },
            adminMgr.HasAnyPermission(adminSid, "cdoe"))
        .AddSubmenu(
            tr.Get("category.control", adminSlot), [adminSlot](int) { return Menu::BuildControlMenu(adminSlot); },
            adminMgr.HasAnyPermission(adminSid, "bsz"))
        .AddSubmenu(
            tr.Get("category.effects", adminSlot), [adminSlot](int) { return Menu::BuildEffectsMenu(adminSlot); },
            adminMgr.HasAnyPermission(adminSid, "fjz"))
        .AddSubmenu(
            tr.Get("category.chatSettings", adminSlot),
            [adminSlot](int) { return Menu::BuildChatSettingsMenu(adminSlot); }, adminMgr.IsAdmin(adminSid))
        .Build();
}

}  // namespace AdminSystem::Admin
