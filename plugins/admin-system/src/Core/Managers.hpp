#pragma once

#include "../Admin/AdminManager.hpp"
#include "../Admin/CheatCheck/CheatCheckManager.hpp"
#include "../Admin/Effects/EffectManager.hpp"
#include "../Database/Database.hpp"
#include "../Database/Repositories/PlayerRepository.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "ChatService.hpp"
#include "Config.hpp"

#include <CS2Kit/Http/HttpClient.hpp>

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
    Database::Database Db;
    Database::PlayerRepository PlayerRepo;
    Admin::AdminManager Admins;
    Punishments::PunishmentManager Punishments;
    Core::ChatService Chat;
    Admin::Effects::EffectManager Effects;
    Admin::CheatCheck::CheatCheckManager CheatCheck;
    CS2Kit::Http::HttpClient Http;
};

/** The plugin's live managers. Valid only between OnLoad and unload. */
Managers& App();

}  // namespace AdminSystem
