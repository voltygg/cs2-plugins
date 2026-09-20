#pragma once

#include "Admin/Menu/MenuContext.hpp"

#include <VoltMod/Menu/Menu.hpp>
#include <memory>
#include <string>

namespace AdminSystem::Admin::Menu
{

/** What picking a map from the list does. */
enum class MapVerb
{
    ChangeNow,
    SetNext,
    PutToVote,
};

/** The configured map cycle under @p title, where picking a map applies @p verb to it. */
std::shared_ptr<VoltMod::Menu> BuildMapPicker(const MenuContext& ctx, MapVerb verb, const std::string& title);

}  // namespace AdminSystem::Admin::Menu
