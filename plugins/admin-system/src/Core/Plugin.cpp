#include "Plugin.hpp"

#include "../Admin/AdminManager.hpp"
#include "../Admin/CheatCheck/CheatCheckManager.hpp"
#include "../Admin/Effects/EffectManager.hpp"
#include "../Commands/AdminCommands.hpp"
#include "../Database/Database.hpp"
#include "../Database/Migrator.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "ChatService.hpp"
#include "Config.hpp"
#include "Managers.hpp"

#include <CS2Kit/Commands/CommandManager.hpp>
#include <CS2Kit/Core/ActiveService.hpp>
#include <CS2Kit/Core/Scheduler.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Http/HttpClient.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Players/Player.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/GameEventService.hpp>
#include <CS2Kit/Sdk/GameInterfaces.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <string>

using namespace AdminSystem::Core;
using namespace AdminSystem::Admin;
using namespace AdminSystem::Admin::Effects;
using namespace AdminSystem::Punishments;
using namespace CS2Kit::Commands;
using namespace CS2Kit::Core;
using namespace CS2Kit::Players;
using namespace CS2Kit::Sdk;
using namespace CS2Kit::Utils;
using namespace CS2Kit::Menu;
using AdminSystem::App;
using AdminSystem::Admin::CheatCheck::CheatCheckManager;
using AdminSystem::Database::Database;
using CS2Kit::Http::HttpClient;

AdminSystemPlugin g_AdminSystemPlugin;
PLUGIN_EXPOSE(AdminSystemPlugin, g_AdminSystemPlugin);

SH_DECL_HOOK3(IVEngineServer2, SetClientListening, SH_NOATTRIB, 0, bool, CPlayerSlot, CPlayerSlot, bool);

AdminSystemPlugin::~AdminSystemPlugin() = default;

AdminSystemPlugin& AdminSystemPlugin::Get()
{
    return g_AdminSystemPlugin;
}

using CS2Kit::Core::Engine;

namespace AdminSystem
{
Managers& App()
{
    return CS2Kit::Core::ActiveService<Managers>::Get();
}
}  // namespace AdminSystem

// ------- Subsystem wiring -----

namespace
{

bool LoadConfigs()
{
    if (!App().Config.LoadSettings("addons/admin-system/configs/settings.jsonc"))
    {
        Log::Error("Failed to load settings.jsonc -- aborting load.");
        return false;
    }
    return true;
}

void InstallCommandCallbacks()
{
    auto& cmdMgr = Engine().Commands;

    cmdMgr.SetPermissionCallback([](int64_t steamId, const std::string& permission) -> bool {
        return App().Admins.HasAnyPermission(steamId, permission);
    });

    // Pipe every command's result message into the player's chat as a colored reply.
    // Suppresses empty messages (e.g. !who, which already streamed its own lines).
    cmdMgr.SetResultCallback([](Player* caller, const Command& /*cmd*/, const CommandResult& result) {
        if (!caller || result.Message.empty())
            return;
        App().Chat.Reply(caller->GetSlot(), result.Message);
    });
}

bool ConnectDatabaseAndLoadAdmins()
{
    Log::Info("Connecting to database...");
    auto& db = App().Db;

    if (!db.Initialize(App().Config.GetDatabase()))
    {
        Log::Warn("Database unavailable - admins/groups not loaded; chat commands will reject all callers.");
        return false;
    }

    if (!AdminSystem::Database::RunMigrations(db, "addons/admin-system/configs/migrations"))
    {
        Log::Warn("Database migrations failed - not loading admins against an out-of-date schema.");
        return false;
    }

    Log::Info("Loading admins from database...");
    auto& adminMgr = App().Admins;
    if (!adminMgr.LoadGroups())
        Log::Warn("Failed to load admin groups from DB.");
    if (!adminMgr.LoadAdmins())
        Log::Warn("Failed to load admins from DB.");
    return true;
}

void RegisterPunishmentTasks()
{
    Log::Info("Loading active punishments...");
    if (!App().Punishments.LoadActivePunishments())
        Log::Warn("Failed to load active punishments.");

    // Sweep expired bans/voice-mutes/text-mutes every minute so timed punishments self-clear without
    // requiring a server restart or manual intervention.
    Engine().Scheduler.Repeat(60'000, []() { App().Punishments.ExpireOldPunishments(); });
}

void RegisterGameEventListeners()
{
    auto& events = Engine().Events;
    events.Listen("player_death", [](IGameEvent* e) {
        if (!e)
            return;
        // GetInt("userid") yields the connection userid (drifts from the slot on reconnect);
        // GetPlayerSlot decodes it to the actual slot.
        int victim = e->GetPlayerSlot("userid").Get();
        if (victim >= 0)
            App().Effects.CancelAllForSlot(victim);
    });
    events.Listen("round_end", [](IGameEvent*) { App().Effects.CancelAllForRoundEnd(); });
    events.Listen("round_prestart", [](IGameEvent*) { App().Effects.CancelAllForRoundEnd(); });
}

// Persist a finished session; shared by the disconnect hook and the unload sweep. No-ops for bots.
void FlushPlayerSession(Player* player)
{
    if (player)
        App().PlayerRepo.RecordDisconnect(player->GetSteamID(), player->GetName(), player->GetPlaytime());
}

}  // namespace

// ------- Plugin lifecycle -----

PluginInfo AdminSystemPlugin::Info() const
{
    return PluginInfo{
        .Name = "Admin System",
        .Author = "Sukhrob Ilyosbekov",
        .Description = "Admin System for CS2",
        .Url = "https://github.com/m9snoi-net/admin-system",
        .License = "MIT",
        .Version = "1.0.0",
        .LogTag = "ADMIN",
    };
}

bool AdminSystemPlugin::OnLoad(bool late)
{
    Log::Info("Loading v{}...", Info().Version);

    // Construct the plugin's services up front so every App() access below resolves to a
    // freshly-built manager (no state carried over from a previous load).
    _managers = std::make_unique<AdminSystem::Managers>();
    CS2Kit::Core::ActiveService<AdminSystem::Managers>::Set(_managers.get());

    Log::Info("Loading configurations...");
    if (!LoadConfigs())
        return false;

    auto locale = App().Config.GetPlugin().locale;
    Log::Info("Translations: Setting language to {}...", locale);
    Engine().Translations.SetLanguage(locale);

    InstallCommandCallbacks();

    // Freeze the player while an admin menu is open so WASD navigation doesn't also walk them around.
    Engine().Menus.SetFreezePlayer(true);

    bool dbConnected = ConnectDatabaseAndLoadAdmins();
    if (dbConnected)
        Defer([] { App().Db.CloseConnection(); });

    Log::Info("Initializing commands...");
    AdminSystem::Commands::RegisterAdminCommands(Engine().Commands);

    if (dbConnected)
        RegisterPunishmentTasks();

    Log::Info("Loading translations...");
    Engine().Translations.Load("addons/admin-system/configs/translations");

    RegisterGameEventListeners();

    // Async HTTP for cheat-check website rooms. Completions are dispatched on the game thread.
    App().Http.Start();
    Defer([] { App().Http.Stop(); });
    uint64_t dispatchTimer = Engine().Scheduler.Repeat(100, [] { App().Http.DispatchCompletions(); });
    Defer([dispatchTimer] { Engine().Scheduler.Cancel(dispatchTimer); });

    Defer([] { App().CheatCheck.CancelAll(); });
    Defer([] { App().Effects.CancelAll(); });
    // Unload fires no disconnect hooks, so fold open sessions here or lose their playtime.
    Defer([] {
        for (auto* p : Engine().Players.GetAllPlayers())
            FlushPlayerSession(p);
    });
    Defer([] { Engine().Players.Clear(); });

    Log::Info("All subsystems initialized.");
    return true;
}

void AdminSystemPlugin::OnDestroyInstances()
{
    // Runs after Defer() cleanups (which still saw live managers) and before the kit's services
    // are destroyed. Dropping these here is what gives a meta reload clean state.
    CS2Kit::Core::ActiveService<AdminSystem::Managers>::Set(nullptr);
    _managers.reset();
}

void AdminSystemPlugin::OnPlayerConnect(Player* player)
{
    if (!player)
        return;

    App().PlayerRepo.RecordConnect(player->GetSteamID(), player->GetName(), player->GetIpAddress());

    // Register the admin's panel language up front so every slot-aware Translations::Get (menus,
    // cheat-check, mute notices) renders in their language without per-command setup.
    if (const auto* row = App().Admins.GetAdmin(player->GetSteamID()))
        Engine().Translations.SetPlayerLanguage(player->GetSlot(), row->Language);

    // Reject banned players. Kicking inside the connect hook is unsafe in some builds, so we defer
    // to the next game frame via the scheduler -- the player is fully connected by then. Bots have
    // no real SteamID and never match an active ban.
    if (auto ban = App().Punishments.GetActiveBan(player->GetSteamID()))
    {
        int slot = player->GetSlot();
        std::string reason = ban->Reason;
        Engine().Scheduler.NextTick([slot, reason]() { PlayerController(slot).Kick(reason.c_str()); });
    }
}

void AdminSystemPlugin::OnPlayerDisconnect(Player* player)
{
    if (player)
    {
        FlushPlayerSession(player);
        App().Effects.CancelAllForSlot(player->GetSlot());
        App().CheatCheck.CancelAllForSlot(player->GetSlot());
    }
}

bool AdminSystemPlugin::OnPlayerChat(Player* player, std::string_view message, bool teamChat)
{
    return App().Chat.HandleSay(player, std::string(message), teamChat);
}

void AdminSystemPlugin::OnRegisterHooks()
{
    auto& gi = Engine().Interfaces;
    SH_ADD_HOOK(IVEngineServer2, SetClientListening, gi.Engine,
                SH_MEMBER(this, &AdminSystemPlugin::Hook_SetClientListening), false);

    Defer([this] {
        auto& g = Engine().Interfaces;
        SH_REMOVE_HOOK(IVEngineServer2, SetClientListening, g.Engine,
                       SH_MEMBER(this, &AdminSystemPlugin::Hook_SetClientListening), false);
    });
}

bool AdminSystemPlugin::Hook_SetClientListening(CPlayerSlot iReceiver, CPlayerSlot iSender, bool bListen)
{
    if (bListen)
    {
        if (auto* sender = Engine().Players.GetPlayerBySlot(iSender.Get()))
        {
            if (App().Punishments.IsVoiceMuted(sender->GetSteamID()))
            {
                // Tell the muted player they're being suppressed; ChatService rate-limits this
                // so the per-receiver explosion of hook calls collapses to one chat line.
                App().Chat.NotifyVoiceMuted(sender);
                RETURN_META_VALUE_NEWPARAMS(MRES_HANDLED, false, &IVEngineServer2::SetClientListening,
                                            (iReceiver, iSender, false));
            }
        }
    }
    RETURN_META_VALUE(MRES_IGNORED, true);
}
