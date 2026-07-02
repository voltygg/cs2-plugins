#pragma once

#include "../../Core/Permissions.hpp"

#include <CS2Kit/Players/Player.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace AdminSystem::Admin::Actions
{

/** A translation key returned by an action body, or nullopt to skip the broadcast. */
using OptKey = std::optional<std::string>;

struct ActionContext
{
    CS2Kit::Players::Player* Admin;
    CS2Kit::Players::Player* Target;
    CS2Kit::Sdk::PlayerController AdminCtrl;
    CS2Kit::Sdk::PlayerController TargetCtrl;

    bool Valid() const { return Admin && Target && TargetCtrl.IsValid(); }
};

/**
 * @brief Resolve admin + target slot pair into an ActionContext, applying
 * AdminManager::CanTarget immunity check. Returns a context with `Valid() == false`
 * if either player is missing, the admin lacks the requested permission, or
 * immunity blocks the action.
 *
 * Pass '\0' for `requiredFlag` to skip the permission check (used internally
 * after the menu/command layer has already enforced it).
 */
ActionContext Resolve(int adminSlot, int targetSlot, char requiredFlag);

void Broadcast(const ActionContext& ctx, const std::string& translationKey);

/** Two-target variant: the phrase at `translationKey` receives the target names as {a} and {b}. */
void Broadcast(const ActionContext& first, const ActionContext& second, const std::string& translationKey);

/**
 * @brief A single-target admin action expressed as data.
 *
 * Bodies receive a valid, permission/immunity-checked @ref ActionContext, mutate the
 * target, and return the broadcast key (or nullopt to stay silent). The dispatcher owns
 * the `Resolve -> guard -> Broadcast` shape, so an action is just its flag, its guards,
 * and its effect - no per-action wrapper function. Menus and commands invoke them through
 * @ref Run.
 */
struct Action
{
    Permission Flag;
    bool RequireAlive = false; /**< Skip silently if the target is dead. */
    std::function<OptKey(const ActionContext&)> Body;
};

/** Like @ref Action but carries an integer the menu/command supplies (health, team, ...). */
struct ParamAction
{
    Permission Flag;
    bool RequireAlive = false;
    std::function<OptKey(const ActionContext&, int param)> Body;
};

void Run(int adminSlot, int targetSlot, const Action& action);
void Run(int adminSlot, int targetSlot, int param, const ParamAction& action);

}  // namespace AdminSystem::Admin::Actions
