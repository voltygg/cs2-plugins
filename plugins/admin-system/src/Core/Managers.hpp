#pragma once

#include "../Admin/AdminManager.hpp"
#include "../Admin/CheatCheck/CheatCheckManager.hpp"
#include "../Admin/FreezeManager.hpp"
#include "../Database/Repositories/PlayerRepository.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "ChatService.hpp"
#include "Config.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/EffectManager.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Database/PostgresDatabase.hpp>
#include <CS2Kit/Players/ActionDispatcher.hpp>

namespace AdminSystem
{

struct Managers;

/** The plugin's live managers. Valid only between OnLoad and unload. */
Managers& App();

/**
 * Plugin-owned service managers, constructed in AdminSystemPlugin::OnLoad and destroyed in
 * OnDestroyInstances - so their state cannot leak across `meta unload`/`meta reload`.
 * Declared in dependency order; destroyed in reverse. Reach them via App().
 */
struct Managers
{
    Core::ConfigManager Config;
    CS2Kit::PostgresDatabase Db;
    Database::PlayerRepository PlayerRepo;
    Admin::AdminManager Admins;
    Admin::FreezeManager Freeze;
    Punishments::PunishmentManager Punishments;
    Core::ChatService Chat;
    // Constructed in OnLoad, when the kit's Services (and its Scheduler) are already live.
    CS2Kit::EffectManager Effects{CS2Kit::Engine().Scheduler};
    Admin::CheatCheck::CheatCheckManager CheatCheck;
    // Kit action dispatcher wired to this plugin's policy. The lambdas capture nothing and
    // resolve App() at call time, so declaring this after Admins/Chat carries no init-order risk.
    CS2Kit::Players::ActionDispatcher ActionDisp{
        [](CS2Kit::Player& caller, const std::string& permission) {
            return App().Admins.HasAnyPermission(caller.GetSteamID(), permission);
        },
        [](CS2Kit::Player& caller, CS2Kit::Player& target) {
            return App().Admins.CanTarget(caller.GetSteamID(), target.GetSteamID());
        },
        [](const CS2Kit::ActionContext& ctx, const std::string& key) {
            App().Chat.BroadcastAction(key, ctx.Caller->GetName(), ctx.Target->GetName());
        }};
};

}  // namespace AdminSystem
