#pragma once

#include "../Admin/AdminManager.hpp"
#include "../Admin/CheatCheck/CheatCheckManager.hpp"
#include "../Admin/FreezeManager.hpp"
#include "../Commands/AntiCheatBridge.hpp"
#include "../Database/Repositories/PlayerRepository.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "ChatService.hpp"
#include "Config.hpp"
#include "StatusCommand.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/EffectManager.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Database/PostgresDatabase.hpp>

namespace AdminSystem
{

struct Managers;

/** The plugin's live managers. Valid only between OnLoad and unload. */
Managers& App();

/**
 * Plugin-owned service managers, constructed by PluginBase (OnCreateInstances) and destroyed in
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
    // Constructed after the kit's Services (and its Scheduler) are live - PluginBase guarantees it.
    CS2Kit::EffectManager Effects{CS2Kit::Engine().Scheduler};
    Admin::CheatCheck::CheatCheckManager CheatCheck;
    Core::AntiCheatBridge AntiCheat;
    Core::StatusCommand Status;
};

}  // namespace AdminSystem
