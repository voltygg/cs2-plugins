#pragma once

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Menu/Menu.hpp>
#include <functional>
#include <memory>
#include <string>

namespace AdminSystem
{
struct App;
}

namespace AdminSystem::Admin::Menu
{

/**
 * @brief Build a paginated picker listing every connected player.
 * Selecting a player invokes onPick(adminSlot, targetSlot) which is expected
 * to open the appropriate per-target actions submenu. @p isEnabled, when supplied,
 * decides per-row whether a target is selectable (e.g. to gray out an already-picked player).
 */
std::shared_ptr<CS2Kit::MenuView> BuildPlayerPicker(AdminSystem::App& app, int adminSlot, const std::string& title,
                                                    std::function<void(int adminSlot, int targetSlot)> onPick,
                                                    std::function<bool(int targetSlot)> isEnabled = {});

}  // namespace AdminSystem::Admin::Menu
