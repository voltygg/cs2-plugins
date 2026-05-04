#include "Bootstrap.hpp"

#include "../Admin/AdminManager.hpp"
#include "../Admin/Effects/EffectManager.hpp"
#include "../Commands/AdminCommands.hpp"
#include "../Database/Database.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "ChatService.hpp"
#include "Config.hpp"

#include <CS2Kit/Commands/CommandManager.hpp>
#include <CS2Kit/Core/Scheduler.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/GameEventService.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <CS2Kit/Utils/Translations.hpp>

namespace AdminSystem::Core::Bootstrap
{

using namespace AdminSystem::Admin;
using namespace AdminSystem::Admin::Effects;
using namespace AdminSystem::Punishments;
using namespace CS2Kit::Commands;
using namespace CS2Kit::Core;
using namespace CS2Kit::Players;
using namespace CS2Kit::Sdk;
using namespace CS2Kit::Utils;
using AdminSystem::Database::Database;

namespace
{

bool LoadConfigs()
{
    auto& configMgr = ConfigManager::Instance();
    if (!configMgr.LoadSettings("addons/admin-system/configs/settings.json"))
        Log::Warn("Failed to load settings.json");
    return true;
}

void InstallCommandCallbacks()
{
    auto& cmdMgr = CommandManager::Instance();

    cmdMgr.SetPermissionCallback([](int64_t steamId, const std::string& permission) -> bool {
        return AdminManager::Instance().HasAnyPermission(steamId, permission);
    });

    // Pipe every command's result message into the player's chat as a colored reply.
    // Suppresses empty messages (e.g. !who, which already streamed its own lines).
    cmdMgr.SetResultCallback([](Player* caller, const Command& /*cmd*/, const CommandResult& result) {
        if (!caller || result.Message.empty())
            return;
        ChatService::Instance().Reply(caller->GetSlot(), result.Message);
    });
}

bool ConnectDatabaseAndLoadAdmins()
{
    Log::Info("Connecting to database...");
    auto& db = Database::Instance();
    auto& configMgr = ConfigManager::Instance();

    bool dbConnected = db.Initialize(configMgr.GetDatabaseConfig());
    if (!dbConnected)
    {
        Log::Warn("Database unavailable -- admins/groups not loaded; chat commands will reject all callers.");
        return false;
    }

    Log::Info("Loading admins from database...");
    auto& adminMgr = AdminManager::Instance();
    if (!adminMgr.LoadGroups())
        Log::Warn("Failed to load admin groups from DB.");
    if (!adminMgr.LoadAdmins())
        Log::Warn("Failed to load admins from DB.");
    return true;
}

void RegisterPunishmentTasks()
{
    Log::Info("Loading active punishments...");
    if (!PunishmentManager::Instance().LoadActivePunishments())
        Log::Warn("Failed to load active punishments.");

    // Sweep expired bans/voice-mutes/text-mutes every minute so timed punishments self-clear without
    // requiring a server restart or manual intervention.
    Scheduler::Instance().Repeat(60'000, []() { PunishmentManager::Instance().ExpireOldPunishments(); });
}

void RegisterGameEventListeners()
{
    auto& events = GameEventService::Instance();
    events.Listen("player_death", [](IGameEvent* e) {
        if (!e)
            return;
        // userid in CS2 events maps to the slot index for the legacy event system.
        int victim = e->GetInt("userid", -1);
        if (victim >= 0)
            EffectManager::Instance().CancelAllForSlot(victim);
    });
    events.Listen("round_end", [](IGameEvent*) { EffectManager::Instance().CancelAllForRoundEnd(); });
    events.Listen("round_prestart", [](IGameEvent*) { EffectManager::Instance().CancelAllForRoundEnd(); });
}

}  // namespace

bool Initialize()
{
    Log::Info("Loading configurations...");
    if (!LoadConfigs())
        return false;

    auto locale = ConfigManager::Instance().GetPluginConfig().Locale;
    Log::Info("Translations: Setting language to {}...", locale);
    Translations::Instance().SetLanguage(locale);

    InstallCommandCallbacks();

    bool dbConnected = ConnectDatabaseAndLoadAdmins();

    Log::Info("Initializing commands...");
    AdminSystem::Commands::RegisterAdminCommands(CommandManager::Instance());

    if (dbConnected)
        RegisterPunishmentTasks();

    Log::Info("Loading translations...");
    Translations::Instance().Load("addons/admin-system/configs/translations");

    RegisterGameEventListeners();

    Log::Info("All subsystems initialized.");
    return true;
}

void Shutdown()
{
    Log::Info("Shutting down subsystems...");
    EffectManager::Instance().CancelAll();
    PlayerManager::Instance().Clear();
    Database::Instance().CloseConnection();
    Log::Info("Subsystems shut down.");
}

}  // namespace AdminSystem::Core::Bootstrap
