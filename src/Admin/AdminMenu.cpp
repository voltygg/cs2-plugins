#include "AdminMenu.hpp"

#include "AdminManager.hpp"
#include "Menu/AdminMenu_ChatSettings.hpp"
#include "Menu/AdminMenu_Control.hpp"
#include "Menu/AdminMenu_Effects.hpp"
#include "Menu/AdminMenu_Punish.hpp"

#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/Translations.hpp>

namespace AdminSystem::Admin
{

using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Players::PlayerManager;
using CS2Kit::Utils::Translations;

std::shared_ptr<::CS2Kit::Menu::Menu> BuildAdminMainMenu(int adminSlot)
{
    auto& tr = Translations::Instance();
    auto& adminMgr = AdminManager::Instance();
    auto& plrMgr = PlayerManager::Instance();

    auto* adminPlayer = plrMgr.GetPlayerBySlot(adminSlot);
    if (!adminPlayer)
        return nullptr;

    int64_t adminSid = adminPlayer->GetSteamID();

    return MenuBuilder(tr.Get("adminPanel"))
        .AddSubmenu(
            tr.Get("categoryPunish"), [adminSlot](int) { return Menu::BuildPunishMenu(adminSlot); },
            adminMgr.HasAnyPermission(adminSid, "cdopq"))
        .AddSubmenu(
            tr.Get("categoryControl"), [adminSlot](int) { return Menu::BuildControlMenu(adminSlot); },
            adminMgr.HasAnyPermission(adminSid, "bsz"))
        .AddSubmenu(
            tr.Get("categoryEffects"), [adminSlot](int) { return Menu::BuildEffectsMenu(adminSlot); },
            adminMgr.HasAnyPermission(adminSid, "fz"))
        .AddSubmenu(
            tr.Get("categoryChatSettings"), [adminSlot](int) { return Menu::BuildChatSettingsMenu(adminSlot); },
            adminMgr.IsAdmin(adminSid))
        .Build();
}

}  // namespace AdminSystem::Admin
