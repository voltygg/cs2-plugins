#include "Plugin.hpp"

#include "../Admin/AdminManager.hpp"
#include "../Admin/CheatCheck/CheatCheckManager.hpp"
#include "../Admin/Effects/Model.hpp"
#include "../Database/Repositories/ServerRepository.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "ChatService.hpp"
#include "Config.hpp"
#include "Managers.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Commands/CommandManager.hpp>
#include <CS2Kit/Core/HookMacros.hpp>
#include <CS2Kit/Core/Scheduler.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Database/Migrator.hpp>
#include <CS2Kit/Database/PostgresDatabase.hpp>
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
using namespace AdminSystem::Punishments;
using namespace CS2Kit::Commands;
using namespace CS2Kit::Core;
using namespace CS2Kit::Players;
using namespace CS2Kit::Sdk;
using namespace CS2Kit::Utils;
using namespace CS2Kit::Menu;
using AdminSystem::App;
using AdminSystem::Admin::CheatCheck::CheatCheckManager;

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
    return AdminSystemPlugin::App();
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

// The one policy the kit consults everywhere: command permissions, action targeting,
// result replies, and action broadcasts. Lambdas resolve App() at call time.
void InstallPolicy()
{
    Engine().Policy = {
        .HasPermission = [](int64_t steamId,
                            const std::string& permission) { return App().Admins.HasAnyPermission(steamId, permission); },
        .CanTarget = [](Player& caller,
                        Player& target) { return App().Admins.CanTarget(caller.GetSteamID(), target.GetSteamID()); },
        .Reply = [](int slot, std::string_view message) { App().Chat.Reply(slot, message); },
        .Broadcast =
            [](Player& caller, Player* target, const std::string& key) {
                if (target)
                    App().Chat.BroadcastAction(key, caller.GetName(), target->GetName());
            },
    };
}

bool ConnectDatabaseAndLoadAdmins()
{
    Log::Info("Connecting to database...");
    auto& db = App().Db;

    if (!db.Start(App().Config.GetDatabase()))
    {
        Log::Warn("Database unavailable - admins/groups not loaded; chat commands will reject all callers.");
        return false;
    }

    if (!CS2Kit::RunMigrations(db, "addons/admin-system/configs/migrations",
                               {.TableName = "schema_migrations", .AdvisoryLockKey = 727274}))
    {
        Log::Warn("Database migrations failed - not loading admins against an out-of-date schema.");
        return false;
    }

    const auto& server = App().Config.GetServer();
    if (!AdminSystem::Database::ServerRepository{}.Upsert(server.tag, server.name))
        Log::Warn("Failed to register server '{}' in the servers table.", server.tag);

    Log::Info("Loading admins from database...");
    auto& adminMgr = App().Admins;
    if (!adminMgr.LoadGroups())
        Log::Warn("Failed to load admin groups from DB.");
    if (!adminMgr.LoadAdmins())
        Log::Warn("Failed to load admins from DB.");
    App().Freeze.RefreshFromDatabase();
    return true;
}

void RegisterPunishmentTasks()
{
    Log::Info("Loading active punishments...");
    if (!App().Punishments.LoadActivePunishments())
        Log::Warn("Failed to load active punishments.");

    // Every minute: sweep expired bans/mutes, pick up admin freezes issued on other servers
    // sharing this database, and advance this server's registry heartbeat.
    Engine().Scheduler.Repeat(60'000, []() {
        App().Punishments.ExpireOldPunishments();
        App().Freeze.RefreshFromDatabase();
        AdminSystem::Database::ServerRepository{}.Heartbeat(App().Config.GetServer().tag);
    });
}

void RegisterGameEventListeners()
{
    namespace Events = CS2Kit::Events;
    auto& events = Engine().Events;
    events.Listen<Events::PlayerDeath>([](const Events::PlayerDeath& e) {
        if (e.VictimSlot >= 0)
            App().Effects.CancelAllForSlot(e.VictimSlot);
    });
    events.Listen<Events::RoundEnd>([](const Events::RoundEnd&) { App().Effects.CancelRoundScoped(); });
    events.Listen<Events::RoundPrestart>([](const Events::RoundPrestart&) { App().Effects.CancelRoundScoped(); });
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
        .Url = "https://github.com/m9snoi-net/cs2-plugins",
        .License = "MIT",
        .Version = "1.0.0",
        .LogTag = "ADMIN",
    };
}

bool AdminSystemPlugin::OnLoad(bool late)
{
    Log::Info("Loading v{}...", Info().Version);

    Log::Info("Loading configurations...");
    if (!LoadConfigs())
        return false;

    auto locale = App().Config.GetPlugin().locale;
    Log::Info("Translations: Setting language to {}...", locale);
    Engine().Translations.SetLanguage(locale);

    InstallPolicy();

    // Freeze the player while an admin menu is open so WASD navigation doesn't also walk them around.
    Engine().Menus.SetFreezePlayer(true);

    bool dbConnected = ConnectDatabaseAndLoadAdmins();
    if (dbConnected)
        // Stop drains queued writes (a ban issued just before unload must land) and drops
        // undispatched completions before the managers they would touch are destroyed.
        Defer([] { App().Db.Stop(); });

    Log::Info("Initializing commands...");
    // Every command self-registered into the Registry at its definition site; ingest once.
    Engine().Commands.RegisterAll(CS2Kit::Registry<CS2Kit::CommandSpec>::Items());

    if (dbConnected)
        RegisterPunishmentTasks();

    Log::Info("Loading translations...");
    Engine().Translations.Load("addons/admin-system/configs/translations");

    RegisterGameEventListeners();

    // Queue fun-model assets; they replicate to clients from the next map load.
    AdminSystem::Admin::Effects::PrecacheModels();

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

void AdminSystemPlugin::OnPlayerConnect(Player* player)
{
    if (!player)
        return;

    App().PlayerRepo.RecordConnect(player->GetSteamID(), player->GetName(), player->GetIpAddress());

    // Register the admin's panel language up front so every slot-aware Translations::Get (menus,
    // cheat-check, mute notices) renders in their language without per-command setup.
    if (const auto* row = App().Admins.GetAdmin(player->GetSteamID()))
        Engine().Translations.SetPlayerLanguage(player->GetSlot(), row->Language);

    // A frozen admin gets told up front instead of discovering it on their first denied command.
    // Deferred a tick like the ban kick below so the freshly-connected client receives the line.
    if (App().Freeze.IsFrozen(player->GetSteamID()))
    {
        int64_t steamId = player->GetSteamID();
        Engine().Scheduler.NextTick([steamId]() { App().Freeze.NotifyFrozen(steamId); });
    }

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
    CS2KIT_SCOPED_HOOK(IVEngineServer2, SetClientListening, Engine().Interfaces.Engine,
                       SH_MEMBER(this, &AdminSystemPlugin::Hook_SetClientListening), false);
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
