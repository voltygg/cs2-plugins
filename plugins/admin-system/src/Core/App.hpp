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

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/EffectManager.hpp>
#include <VoltMod/Database/Api.hpp>
#include <string>
#include <vector>

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
    App(VoltMod::Runtime& runtime, std::string version) : Runtime(runtime), Version(std::move(version)) {}
    ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /** Connect the database, run migrations, load admins and register commands. */
    bool Start();

    /** Persist a finished session; shared by the disconnect hook and the unload sweep. */
    void FlushPlayerSession(VoltMod::Player* player);

    VoltMod::Runtime& Runtime;
    /** Plugin version, for the menu title. */
    const std::string Version;

    Core::ConfigManager Config;
    VoltMod::PostgresDatabase Db;
    Database::PlayerRepository PlayerRepo{Db};
    Core::ChatService Chat{Runtime, Config};
    Admin::AdminManager Admins{Db, Config};
    Admin::FreezeManager Freeze{Db, Config, Runtime, Chat, Admins};
    /** The permission gate: granted flags minus abuse-protection freezes. Ask this, not Admins. */
    Admin::Access Access{Admins, Freeze};
    Punishments::PunishmentManager Punishments{Db, Config, Runtime, Chat};
    Core::PlayerChat PlayerChat{Runtime, Config, Chat, Admins, Punishments};
    Reports::ReportManager Reports{Db, Config, Runtime};
    VoltMod::EffectManager Effects{Runtime.Scheduler};
    Admin::CheatCheck::CheatCheckManager CheatCheck{Runtime, Config, Chat};
    /** Published to other plugins in Start; withdrawn before these managers die. */
    Core::AdminActionsService AdminActions{Runtime, Punishments, Access};
    /** Load-time migration outcome, surfaced in the `admin_status` db section. */
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
