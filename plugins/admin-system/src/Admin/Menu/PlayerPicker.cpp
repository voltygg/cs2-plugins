#include "PlayerPicker.hpp"

#include "../../Core/App.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Translations.hpp>
#include <CS2Kit/Menu/MenuPresets.hpp>
#include <CS2Kit/Runtime.hpp>
#include <utility>

namespace AdminSystem::Admin::Menu
{

std::shared_ptr<CS2Kit::MenuView> BuildPlayerPicker(AdminSystem::App& app, int adminSlot, const std::string& title,
                                                    std::function<void(int adminSlot, int targetSlot)> onPick,
                                                    std::function<bool(int targetSlot)> isEnabled)
{
    auto& tr = app.Runtime.Translations;
    return ::CS2Kit::Menu::BuildPlayerPicker(adminSlot, title, std::move(onPick), tr.Get("common.noPlayers", adminSlot),
                                             std::move(isEnabled));
}

}  // namespace AdminSystem::Admin::Menu
