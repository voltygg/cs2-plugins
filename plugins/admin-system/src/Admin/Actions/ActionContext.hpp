#pragma once

#include "../../Core/Permissions.hpp"

#include <CS2Kit/Players/ActionDispatcher.hpp>
#include <string>

namespace AdminSystem::Admin::Actions
{

// The action scaffold lives in the kit; the plugin supplies its permission/immunity/broadcast
// policy through App().ActionDisp (see Managers.hpp). These aliases keep descriptor files and
// call sites on the established local names.
using ActionContext = CS2Kit::Players::ActionContext;
using Action = CS2Kit::Players::Action;
using ParamAction = CS2Kit::Players::ParamAction;
using OptKey = CS2Kit::Players::OptKey;

/**
 * @brief Resolve admin + target slot pair through the plugin's dispatcher, applying the
 * permission and immunity policies. Pass an empty flag string to skip the permission check
 * (used internally after the menu/command layer has already enforced it).
 */
ActionContext Resolve(int adminSlot, int targetSlot, const std::string& requiredFlag);

void Broadcast(const ActionContext& ctx, const std::string& translationKey);

/** Two-target variant: the phrase at `translationKey` receives the target names as {a} and {b}. */
void Broadcast(const ActionContext& first, const ActionContext& second, const std::string& translationKey);

void Run(int adminSlot, int targetSlot, const Action& action);
void Run(int adminSlot, int targetSlot, int param, const ParamAction& action);

}  // namespace AdminSystem::Admin::Actions
