#pragma once

#include "../Admin/Access.hpp"
#include "../Admin/AdminManager.hpp"
#include "../Admin/CheatCheck/CheatCheckManager.hpp"
#include "../Admin/FreezeManager.hpp"
#include "../Database/Repositories/PlayerRepository.hpp"
#include "../Fun/FunMode.hpp"
#include "../Maps/MapCycleState.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "../Reports/ReportManager.hpp"
#include "AdminActionsService.hpp"
#include "ChatService.hpp"
#include "Config.hpp"
#include "PlayerChat.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/EffectManager.hpp>
#include <VoltMod/Database/Api.hpp>
#include <VoltMod/Players/ActionDispatcher.hpp>
#include <VoltMod/Players/EffectDispatcher.hpp>
#include <string>
#include <vector>

namespace AdminSystem
{

/**
 * Load-cycle object graph. Members are declared in dependency order and destroyed
 * in reverse, so callbacks stop before captured state and database services.
 * Access composes admin flags with freeze state; PlayerChat owns inbound rules
 * while ChatService remains output-only.
 */
struct App
{
    App(VoltMod::Runtime& runtime, std::string version) : Runtime(runtime), Version(std::move(version)) {}
    ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /** Connect the database, run migrations, load admins and register commands. */
    bool Start();

    /** Persist a finished session; shared by the disconnect hook and the unload sweep. */
    void FlushPlayerSession(VoltMod::Player* player);

    VoltMod::Runtime& Runtime;
    const std::string Version;

    Core::ConfigManager Config;
    /** Runs the action descriptors through Runtime::Policy: permissions, targeting and broadcasts. */
    VoltMod::ActionDispatcher Actions{Runtime};
    VoltMod::PostgresDatabase Db{Runtime.Scheduler};
    Database::PlayerRepository PlayerRepo{Db};
    Core::ChatService Chat{Runtime, Config};
    /** Configured map list, the queued next map, and the level change itself. */
    Maps::MapCycleState MapCycle{Runtime, Config};
    /** Server-wide round modifiers (Fun Mode). */
    Fun::FunMode FunMode{*this};
    Admin::AdminManager Admins{Db, Config};
    Admin::FreezeManager Freeze{Db, Config, Runtime, Chat, Admins};
    /** The permission gate: granted flags minus abuse-protection freezes. Ask this, not Admins. */
    Admin::Access Access{Admins, Freeze};
    Punishments::PunishmentManager Punishments{Db, Config, Runtime, Chat};
    Core::PlayerChat PlayerChat{Runtime, Config, Chat, Admins, Punishments};
    Reports::ReportManager Reports{Db, Config, Runtime};
    VoltMod::EffectManager Effects{Runtime.Scheduler};
    /** Runs the effect descriptors through Runtime::Policy: permissions, targeting and broadcasts. */
    VoltMod::EffectDispatcher PlayerEffects{Runtime, Effects};
    Admin::CheatCheck::CheatCheckManager CheatCheck{Runtime, Config, Chat};
    /** Published to other plugins in Start; withdrawn before these managers die. */
    Core::AdminActionsService AdminActions{Runtime, Punishments, Access};
    /** Load-time migration outcome shown by `admin_status`. */
    VoltMod::MigrationResult Migration;

private:
    void InstallPolicy();
    VoltMod::StageResult ConnectDatabase();
    VoltMod::StageResult LoadAdminData();
    VoltMod::StageResult StartPunishments();
    void RegisterGameEventListeners();
    void InstallStatusReporting();
    void RegisterCommands();

    /** Listener registrations, released together. Declared last: reverse member destruction
     *  stops the callbacks before the state they capture goes away. */
    std::vector<VoltMod::Subscription> _subs;
};

}  // namespace AdminSystem
