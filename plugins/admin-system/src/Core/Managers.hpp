#pragma once

#include "../Admin/AdminManager.hpp"
#include "../Admin/CheatCheck/CheatCheckManager.hpp"
#include "../Database/Repositories/PlayerRepository.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "ChatService.hpp"
#include "Config.hpp"

#include <CS2Kit/Core/EffectManager.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Database/PostgresDatabase.hpp>

namespace AdminSystem
{

/**
 * Plugin-owned service managers, constructed in AdminSystemPlugin::OnLoad and destroyed in
 * OnDestroyInstances - so their state cannot leak across `meta unload`/`meta reload`.
 * Declared in dependency order; destroyed in reverse. Reach them via App().
 */
struct Managers
{
    Core::ConfigManager Config;
    CS2Kit::Database::PostgresDatabase Db;
    Database::PlayerRepository PlayerRepo;
    Admin::AdminManager Admins;
    Punishments::PunishmentManager Punishments;
    Core::ChatService Chat;
    // Constructed in OnLoad, when the kit's Services (and its Scheduler) are already live.
    CS2Kit::Core::EffectManager Effects{CS2Kit::Core::Engine().Scheduler};
    Admin::CheatCheck::CheatCheckManager CheatCheck;
};

/** The plugin's live managers. Valid only between OnLoad and unload. */
Managers& App();

}  // namespace AdminSystem
