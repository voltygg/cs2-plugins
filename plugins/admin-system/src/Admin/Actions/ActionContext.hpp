#pragma once

#include "Core/Permissions.hpp"

#include <VoltMod/Players/Player.hpp>
#include <VoltMod/Players/Policy.hpp>
#include <functional>
#include <optional>
#include <string>

namespace AdminSystem::Admin::Actions
{

/** A translation key returned by an action body, or nullopt to skip the broadcast. */
using OptKey = std::optional<std::string>;

/** Resolved caller/target pair handed to action bodies. */
struct ActionContext
{
    /** The pair, as `Policy::Authorize` cleared it. */
    VoltMod::Authorized Auth;

    VoltMod::Player& Caller() const { return Auth.Caller; }
    /** An action always has a target: @ref ActionDispatcher::Resolve fails without one. */
    VoltMod::Player& Target() const { return *Auth.Target; }
};

/**
 * @brief A single-target player action expressed as data.
 *
 * Bodies receive an authorized @ref ActionContext, mutate the target, and return the broadcast
 * key (or nullopt to stay silent). @ref ActionDispatcher owns the `Resolve -> guard -> Broadcast`
 * shape, so an action is just its permission, its guards, and its effect. A body that needs an
 * engine service reaches it through the `App&` it captured, not through this context.
 */
struct Action
{
    std::string Permission;    /**< "" skips the check. */
    bool RequireAlive = false; /**< Skip silently if the target is dead. */
    std::function<OptKey(const ActionContext&)> Body;
};

/** Like @ref Action but carries an integer the menu or command supplies (health, team, ...). */
struct ParamAction
{
    std::string Permission;
    bool RequireAlive = false;
    std::function<OptKey(const ActionContext&, int param)> Body;
};

}  // namespace AdminSystem::Admin::Actions
