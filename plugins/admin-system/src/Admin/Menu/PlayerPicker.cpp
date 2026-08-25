#include "PlayerPicker.hpp"

#include "../../Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/MenuPresets.hpp>
#include <VoltMod/Runtime.hpp>
#include <utility>

namespace AdminSystem::Admin::Menu
{

std::shared_ptr<VoltMod::MenuView> BuildPlayerPicker(AdminSystem::App& app, int adminSlot, const std::string& title,
                                                     std::function<void(int adminSlot, int targetSlot)> onPick,
                                                     std::function<bool(int targetSlot)> isEnabled)
{
    auto& tr = app.Runtime.Translations;
    return ::VoltMod::Menu::BuildPlayerPicker(app.Runtime.Players, adminSlot, title, std::move(onPick),
                                              tr.Get("common.noPlayers", adminSlot), std::move(isEnabled));
}

}  // namespace AdminSystem::Admin::Menu
