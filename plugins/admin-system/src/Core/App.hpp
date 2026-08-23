#pragma once

#include "../Admin/Access.hpp"
#include "../Admin/AdminManager.hpp"
#include "../Admin/CheatCheck/CheatCheckManager.hpp"
#include "../Admin/FreezeManager.hpp"
#include "../Database/Repositories/PlayerRepository.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "../Reports/ReportManager.hpp"
#include "AdminActionsService.hpp"
#include "ChatService.hpp"
#include "Config.hpp"
#include "PlayerChat.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/EffectManager.hpp>
#include <CS2Kit/Database/Api.hpp>
#include <string>

namespace AdminSystem
{

/**
 * Everything this plugin owns for one Load/Unload cycle. The plugin creates it in OnLoad and
 * drops it in OnUnload, so no state survives a `meta reload`.
 *
 * Every member takes the collaborators it actually uses, so this list is the whole object graph
 * and reading a constructor tells you what a manager can reach. Declaration order is dependency
 * order; destruction is the reverse, which is what makes the subscriptions and the database stop
 * before the things their callbacks touch.
 *
 * Two splits exist to keep that order possible at all. @ref Admin::Access composes the flag
 * store and the freeze set instead of letting them call each other, and @ref Core::PlayerChat
 * holds the inbound chat rules that read admin/punishment state while @ref Core::ChatService
 * stays pure output. Free functions (menus, commands, actions) still take this container - they
 * are leaves, and enumerating five managers per builder would cost more than it explains.
 */
struct App
{
    App(CS2Kit::Runtime& runtime, std::string version) : Runtime(runtime), Version(std::move(version)) {}
    ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /** Connect the database, run migrations, load admins and register commands. */
    bool Start();

    /** Persist a finished session; shared by the disconnect hook and the unload sweep. */
    void FlushPlayerSession(CS2Kit::Player* player);

    CS2Kit::Runtime& Runtime;
    /** Plugin version, for the menu title. */
    const std::string Version;

    Core::ConfigManager Config;
    CS2Kit::PostgresDatabase Db;
    Database::PlayerRepository PlayerRepo{Db};
    Core::ChatService Chat{Runtime, Config};
    Admin::AdminManager Admins{Db, Config};
    Admin::FreezeManager Freeze{Db, Config, Runtime, Chat, Admins};
    /** The permission gate: granted flags minus abuse-protection freezes. Ask this, not Admins. */
    Admin::Access Access{Admins, Freeze};
    Punishments::PunishmentManager Punishments{Db, Config, Runtime, Chat};
    Core::PlayerChat PlayerChat{Runtime, Config, Chat, Admins, Punishments};
    Reports::ReportManager Reports{Db, Config, Runtime};
    CS2Kit::EffectManager Effects{Runtime.Scheduler};
    Admin::CheatCheck::CheatCheckManager CheatCheck{Runtime, Config, Chat};
    /** Published to other plugins in Start; withdrawn before these managers die. */
    Core::AdminActionsService AdminActions{Runtime, Punishments, Access};
    /** Load-time migration outcome, surfaced in the `admin_status` db section. */
    CS2Kit::MigrationResult Migration;

private:
    void InstallPolicy();
    CS2Kit::StageResult ConnectDatabase();
    CS2Kit::StageResult LoadAdminData();
    CS2Kit::StageResult StartPunishments();
    void RegisterGameEventListeners();
    void InstallStatusReporting();
    void RegisterCommands();

    CS2Kit::Subscription _maintenance;
    CS2Kit::Subscription _playerDeath;
    CS2Kit::Subscription _roundEnd;
    CS2Kit::Subscription _roundPrestart;
};

}  // namespace AdminSystem
