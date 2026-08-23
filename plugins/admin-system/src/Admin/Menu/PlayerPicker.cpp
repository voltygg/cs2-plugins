#include "PlayerPicker.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/App/Services.hpp>
#include <CS2Kit/Menu/MenuPresets.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <utility>

using CS2Kit::App::Engine;

namespace AdminSystem::Admin::Menu
{

std::shared_ptr<CS2Kit::MenuView> BuildPlayerPicker(int adminSlot, const std::string& title,
                                                    std::function<void(int adminSlot, int targetSlot)> onPick,
                                                    std::function<bool(int targetSlot)> isEnabled)
{
    auto& tr = Engine().Utils.Translations;
    return ::CS2Kit::Menu::BuildPlayerPicker(adminSlot, title, std::move(onPick), tr.Get("common.noPlayers", adminSlot),
                                             std::move(isEnabled));
}

}  // namespace AdminSystem::Admin::Menu
