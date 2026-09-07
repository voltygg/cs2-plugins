#pragma once

#include "../../Core/Permissions.hpp"

#include <VoltMod/Players/ActionDispatcher.hpp>

namespace AdminSystem::Admin::Actions
{

// VoltMod owns dispatch and policy. These aliases keep plugin descriptors on the local names.
using ActionContext = VoltMod::ActionContext;
using Action = VoltMod::Action;
using ParamAction = VoltMod::ParamAction;
using OptKey = VoltMod::OptKey;

}  // namespace AdminSystem::Admin::Actions
