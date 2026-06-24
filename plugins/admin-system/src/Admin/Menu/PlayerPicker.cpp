#include "PlayerPicker.hpp"
#include <CS2Kit/Core/Services.hpp>

#include <CS2Kit/Menu/MenuPresets.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <utility>

using CS2Kit::Core::Kit;

namespace AdminSystem::Admin::Menu
{

std::shared_ptr<::CS2Kit::Menu::Menu> BuildPlayerPicker(
    int adminSlot, const std::string& title, std::function<void(int adminSlot, int targetSlot)> onPick)
{
    auto& tr = Kit().Translations;
    return ::CS2Kit::Menu::BuildPlayerPicker(adminSlot, title, std::move(onPick),
                                             tr.Get("common.noPlayers", adminSlot));
}

}  // namespace AdminSystem::Admin::Menu
