#pragma once

#include "../../Core/Permissions.hpp"

#include <VoltMod/Players/ActionDispatcher.hpp>

namespace AdminSystem::Admin::Actions
{

// The action scaffold lives in the framework; the plugin supplies its permission/immunity/broadcast
// policy through app.Runtime.Policy (set once in OnLoad) and dispatches through app.Actions. These
// aliases keep descriptor files and call sites on the established local names.
using ActionContext = VoltMod::Players::ActionContext;
using Action = VoltMod::Players::Action;
using ParamAction = VoltMod::Players::ParamAction;
using OptKey = VoltMod::Players::OptKey;

}  // namespace AdminSystem::Admin::Actions
