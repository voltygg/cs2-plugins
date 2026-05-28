#include "PlayerPicker.hpp"

#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/Translations.hpp>

namespace AdminSystem::Admin::Menu
{

using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Players::PlayerManager;
using CS2Kit::Utils::Translations;

std::shared_ptr<::CS2Kit::Menu::Menu> BuildPlayerPicker(
    int adminSlot, const std::string& title, std::function<void(int adminSlot, int targetSlot)> onPick)
{
    auto& tr = Translations::Instance();
    auto& plrMgr = PlayerManager::Instance();

    MenuBuilder builder(title);

    auto players = plrMgr.GetAllPlayers();
    for (auto* p : players)
    {
        int targetSlot = p->GetSlot();
        int admin = adminSlot;
        auto cb = onPick;
        builder.AddButton(p->GetName(), [admin, targetSlot, cb](int /*slot*/) { cb(admin, targetSlot); });
    }

    if (players.empty())
    {
        builder.AddButton(tr.Get("common.noPlayers"), [](int) {}, false);
    }

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
