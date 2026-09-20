#pragma once

#include "Admin/Menu/MenuContext.hpp"

#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** What picking a map from the list does. */
enum class MapVerb
{
    ChangeNow,
    SetNext,
    PutToVote,
};

/** The configured map cycle, where picking one applies @p verb to it. */
std::shared_ptr<VoltMod::Menu> BuildMapPicker(const MenuContext& ctx, MapVerb verb);

}  // namespace AdminSystem::Admin::Menu
