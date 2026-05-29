#pragma once

#include <CS2Kit/Players/Player.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace AdminSystem::Admin::Actions
{

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

/**
 * @brief Resolve admin+target (permission + immunity), run @p body when the context is valid,
 * and broadcast the translation key it returns — or skip the broadcast if it returns nullopt.
 *
 * Folds the `Resolve -> if (!Valid()) return -> ... -> Broadcast` shape every single-target
 * action repeats. The body performs any extra guard (IsAlive, etc.) and picks the key, so
 * conditional verbs (godmode on/off) and no-op cases (dead target) stay in one place.
 */
void RunAction(int adminSlot, int targetSlot, char requiredFlag,
               const std::function<std::optional<std::string>(const ActionContext&)>& body);

}  // namespace AdminSystem::Admin::Actions
