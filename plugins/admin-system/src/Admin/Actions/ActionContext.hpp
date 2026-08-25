#pragma once

#include "../../Core/Permissions.hpp"

#include <VoltMod/Players/ActionDispatcher.hpp>
#include <string>

namespace VoltMod
{
class Runtime;
}

namespace AdminSystem
{
struct App;
}

namespace AdminSystem::Admin::Actions
{

// The action scaffold lives in the framework; the plugin supplies its permission/immunity/broadcast
// policy through app.Runtime.Policy (set once in OnLoad). These aliases keep descriptor files and
// call sites on the established local names.
using ActionContext = VoltMod::Players::ActionContext;
using Action = VoltMod::Players::Action;
using ParamAction = VoltMod::Players::ParamAction;
using OptKey = VoltMod::Players::OptKey;

/**
 * @brief Resolve admin + target slot pair through the plugin's dispatcher, applying the
 * permission and immunity policies. Pass an empty flag string to skip the permission check
 * (used internally after the menu/command layer has already enforced it).
 */
ActionContext Resolve(VoltMod::Runtime& runtime, int adminSlot, int targetSlot, const std::string& requiredFlag);

/** Two-target broadcast: the phrase at `translationKey` receives the target names as {a} and {b}. */
void Broadcast(App& app, const ActionContext& first, const ActionContext& second, const std::string& translationKey);

void Run(VoltMod::Runtime& runtime, int adminSlot, int targetSlot, int param, const ParamAction& action);

}  // namespace AdminSystem::Admin::Actions
