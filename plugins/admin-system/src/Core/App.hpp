#pragma once

#include "../Admin/AdminManager.hpp"
#include "../Admin/CheatCheck/CheatCheckManager.hpp"
#include "../Admin/FreezeManager.hpp"
#include "../Database/Repositories/PlayerRepository.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "../Reports/ReportManager.hpp"
#include "AdminActionsService.hpp"
#include "ChatService.hpp"
#include "Config.hpp"

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
 * The managers form a cycle - admin state gates freezing and freezing gates admin state - so
 * each takes this container by reference rather than an enumerated list of siblings. That keeps
 * the dependency in the constructor signature and out of process-global state; commands, menus
 * and effects, which have no such cycle, are handed the specific managers they use.
 *
 * Members are declared in dependency order and destroyed in reverse.
 */
struct App
{
    explicit App(CS2Kit::Runtime& runtime) : Runtime(runtime) {}
    ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /** Connect the database, run migrations, load admins and register commands. */
    bool Start();

    /** Persist a finished session; shared by the disconnect hook and the unload sweep. */
    void FlushPlayerSession(CS2Kit::Player* player);

    CS2Kit::Runtime& Runtime;
    /** Plugin version, for the menu title. Set by the plugin from its Info(). */
    std::string Version;

    Core::ConfigManager Config;
    CS2Kit::PostgresDatabase Db;
    Database::PlayerRepository PlayerRepo{Db};
    Admin::AdminManager Admins{*this};
    Admin::FreezeManager Freeze{*this};
    Punishments::PunishmentManager Punishments{*this};
    Core::ChatService Chat{*this};
    Reports::ReportManager Reports{*this};
    CS2Kit::EffectManager Effects{Runtime.Scheduler};
    Admin::CheatCheck::CheatCheckManager CheatCheck{*this};
    /** Published to other plugins in Start; withdrawn before these managers die. */
    Core::AdminActionsService AdminActions{*this};
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

    CS2Kit::Subscription _playerDeath;
    CS2Kit::Subscription _roundEnd;
    CS2Kit::Subscription _roundPrestart;
};

}  // namespace AdminSystem
