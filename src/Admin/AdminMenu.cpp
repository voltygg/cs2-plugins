#include "AdminMenu.hpp"

#include "AdminManager.hpp"
#include "Menu/AdminMenu_Control.hpp"
#include "Menu/AdminMenu_Effects.hpp"
#include "Menu/AdminMenu_Punish.hpp"

#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/Translations.hpp>

namespace AdminSystem::Admin
{

using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Menu::MenuManager;
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

    MenuBuilder builder(tr.Get("adminPanel"));

    {
        bool hasPerm = adminMgr.HasAnyPermission(adminSid, "cdopq");
        int slot = adminSlot;
        builder.AddItem(
            tr.Get("categoryPunish"),
            [slot](int s) {
                auto m = Menu::BuildPunishMenu(slot);
                if (m)
                    MenuManager::Instance().OpenMenu(s, m);
            },
            hasPerm);
    }

    {
        bool hasPerm = adminMgr.HasAnyPermission(adminSid, "bsz");
        int slot = adminSlot;
        builder.AddItem(
            tr.Get("categoryControl"),
            [slot](int s) {
                auto m = Menu::BuildControlMenu(slot);
                if (m)
                    MenuManager::Instance().OpenMenu(s, m);
            },
            hasPerm);
    }

    {
        bool hasPerm = adminMgr.HasAnyPermission(adminSid, "fz");
        int slot = adminSlot;
        builder.AddItem(
            tr.Get("categoryEffects"),
            [slot](int s) {
                auto m = Menu::BuildEffectsMenu(slot);
                if (m)
                    MenuManager::Instance().OpenMenu(s, m);
            },
            hasPerm);
    }

    return builder.Build();
}

}  // namespace AdminSystem::Admin
