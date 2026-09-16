#include "Core/App.hpp"

#include "Admin/Effects/Model.hpp"
#include "Commands/Commands.hpp"
#include "Config/ConfigManager.hpp"
#include "Punishments/KickNotice.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Database/Api.hpp>
#include <VoltMod/Events/EventTypes.hpp>
#include <string>

using VoltMod::Error;
using VoltMod::Player;
using VoltMod::Status;
namespace Log = VoltMod::Log;

namespace AdminSystem
{

App::~App()
{
    // Stop answering other plugins' MetaFactory queries before the managers it delegates to go.
    AdminActions.Unpublish();
    CheatCheck.CancelAll();
    Effects.CancelAll();
    // Unload fires no disconnect hooks. Clear() raises Players.Disconnected for everyone still
    // connected, and OnPlayerDisconnect is still subscribed here in the destructor body, so open
    // sessions are folded through the one path rather than by a second sweep.
    Runtime.Players.Clear();
    // Drains queued writes (a ban issued just before unload must land) and drops undispatched
    // completions before the managers they would touch are destroyed.
    Db.Stop();
}

// Install the shared command, action, reply, and broadcast policy.
void App::InstallPolicy()
{
    auto& policy = Runtime.Policy;
    policy.HasPermission = [this](int64_t steamId, std::string_view permission) {
        return Access.HasAnyPermission(steamId, std::string(permission));
    };
    // Immunity only: Policy::Authorize has already dealt with the console (no caller) and with a
    // caller targeting themselves before this is consulted. SteamIDs, so the same rule answers
    // for an offline target through Policy::AuthorizeSteamId.
    policy.CanTarget = [this](int64_t callerSteamId, int64_t targetSteamId) {
        return Access.CanTarget(callerSteamId, targetSteamId);
    };
    policy.Reply = [this](int slot, std::string_view message) { Chat.Reply(slot, message); };
    policy.Broadcast = [this](const VoltMod::Authorized& who, std::string_view key) {
        if (!who.Target)
            return;
        // An admin acting on themselves reads as "Bob noclipped" rather than "Bob noclipped Bob".
        const bool onSelf = who.Target->SteamId() == who.Caller.SteamId();
        Chat.BroadcastAction(std::string(key), who.Caller.Name(), onSelf ? std::string_view{} : who.Target->Name());
    };
}

// The connection lifecycle: one subscription per edge, kept in _subs so the handlers stop before
// the managers they touch are destroyed.
void App::RegisterPlayerLifecycle()
{
    _subs.Add(Runtime.Players.Connected += [this](Player& player) { OnPlayerConnect(player); });
    _subs.Add(Runtime.Players.Disconnected += [this](Player& player) { OnPlayerDisconnect(player); });
}

void App::OnPlayerConnect(Player& player)
{
    const int64_t steamId = player.SteamId();
    const int slot = player.Slot();
    Repos.Players.RecordConnectAsync(steamId, player.Name(), std::string(player.Ip()));

    // Register the admin's panel language up front so every slot-aware Translations::Get (menus,
    // cheat-check, mute notices) renders in their language without per-command setup.
    if (const auto* row = Admins.GetAdmin(steamId))
        Runtime.Translations.SetPlayerLanguage(slot, row->Language);

    // A frozen admin gets told up front instead of discovering it on their first denied command.
    if (Freeze.IsFrozen(steamId))
        Freeze.NotifyFrozenSoon(slot, steamId);

    // Reject banned players. Kicking inside the connect hook is unsafe in some builds, so
    // KickDeferred waits a frame -- the player is fully connected by then. Bots have no real
    // SteamID and never match an active ban.
    if (auto ban = Punishments.GetActive(AdminSystem::Punishments::PunishType::Ban, steamId))
    {
        // Built now, while the ban row is in hand, so the disconnect screen carries the expiry
        // and appeal link rather than the bare reason.
        Punishments.KickDeferred(slot, steamId,
                                 AdminSystem::Punishments::BuildBanNotice(Runtime.Translations, Settings.GetAppeal(),
                                                                          ban->Reason, ban->ExpiresAt, steamId, slot));
    }
}

void App::OnPlayerDisconnect(Player& player)
{
    Repos.Players.RecordDisconnectAsync(player.SteamId(), player.Name(), player.Playtime().count());
    Effects.CancelAll(player.Slot());
    CheatCheck.CancelAllForSlot(player.Slot());
}

Status App::ConnectDatabase()
{
    if (!Db.Start(Settings.GetDatabase()))
        return std::unexpected(Error::Engine("unavailable; chat commands will reject all callers"));

    Migration = VoltMod::RunMigrations(Db, VoltMod::AddonFile(Config::AddonName, "configs/migrations"),
                                       {.HistoryTable = "schema_migrations", .LockKey = 727274});
    if (!Migration)
        return std::unexpected(Error::Failed("migrations failed; not loading admins against an out-of-date schema"));

    const auto& server = Settings.GetServer();
    if (!Repos.Servers.Upsert(server.tag, server.name))
        Log::Warn("Failed to register server '{}' in the servers table.", server.tag);

    return {};
}

Status App::LoadAdminData()
{
    const bool groups = Admins.LoadGroups();
    const bool admins = Admins.LoadAdmins();
    Freeze.RefreshFromDatabase();
    if (!groups || !admins)
        return std::unexpected(Error::Failed("failed to load groups/admins from DB"));
    return {};
}

Status App::StartPunishments()
{
    const bool loaded = Punishments.LoadActivePunishments();

    // Every minute: sweep expired bans/mutes, pick up admin freezes issued on other servers
    // sharing this database, and advance this server's registry heartbeat.
    _subs.Add(Runtime.Scheduler.Repeat(60'000, [this] {
        Punishments.ExpireOldPunishments();
        Freeze.RefreshFromDatabase();
        Repos.Servers.HeartbeatAsync(Settings.GetServer().tag);
    }));

    // Typed surface the anticheat plugin drives (bans need the DB, alerts need admin data).
    // Published last in this stage so a peer never sees a half-initialised implementation.
    AdminActions.Publish();

    if (!loaded)
        return std::unexpected(Error::Failed("failed to load active punishments"));
    return {};
}

void App::RegisterGameEventListeners()
{
    auto& events = Runtime.GameEvents;
    _subs.Add(events.On<VoltMod::PlayerDeath>([this](const VoltMod::PlayerDeath& e) {
        // Clear per-life effects; EffectScope::Session grants (e.g. bhop) survive death.
        if (e.VictimSlot >= 0)
            Effects.CancelOnDeath(e.VictimSlot);
    }));
    _subs.Add(events.On<VoltMod::RoundEnd>([this](const VoltMod::RoundEnd&) {
        Effects.CancelRound();
        // A map queued from the menu or by a passing vote lands here rather than mid-round,
        // after a pause long enough to read the scoreboard. No-op when nothing is queued.
        MapCycle.ChangeToNext();
    }));
    _subs.Add(events.On<VoltMod::RoundPrestart>([this](const VoltMod::RoundPrestart&) { Effects.CancelRound(); }));
}

// Add plugin status sections and require a live database for overall health.
void App::InstallStatusReporting()
{
    auto& status = Runtime.Status;

    status.RegisterSection("db", [this] {
        // Live worker state, not the load-time stage result: a database that died (or recovered)
        // after load must show as such.
        return VoltMod::Json::Write(glz::obj{"connected", Db.IsConnected(), "driver",
                                             VoltMod::DriverName(Db.GetDriver()), "migrationVersion",
                                             Migration.CurrentVersion, "migrationsApplied", Migration.Applied});
    });

    status.RegisterSection("admins", [this] {
        return VoltMod::Json::Write(glz::obj{"cached", Admins.AdminCount(), "groups", Admins.GroupCount()});
    });

    status.RegisterSection("commands",
                           [this] { return VoltMod::Json::Write(glz::obj{"registered", Runtime.Commands.Count()}); });

    status.RegisterSection("server", [this] {
        const auto& server = Settings.GetServer();
        return VoltMod::Json::Write(glz::obj{"tag", server.tag, "name", server.name});
    });

    status.InstallCommand("admin_status",
                          "Report plugin health; 'admin_status json' emits a machine-readable STATUS_JSON line.",
                          [this] { return Db.IsConnected(); });
}

void App::RegisterCommands()
{
    auto& commands = Runtime.Commands;
    Commands::RegisterAdminMenuCommand(commands, *this);
    Commands::RegisterAdminSelfCommands(commands, *this);
    Commands::RegisterCheatCheckCommands(commands, *this);
    Commands::RegisterFreezeCommands(commands, *this);
    Commands::RegisterInfoCommands(commands, *this);
    Commands::RegisterPunishmentCommands(commands, *this);
    Commands::RegisterReportCommand(commands, *this);
}

bool App::Start()
{
    if (!VoltMod::LoadStandardConfig(Runtime, Settings, {.Addon = Config::AddonName}))
        return false;

    InstallPolicy();
    RegisterPlayerLifecycle();
    // Freeze the player while an admin menu is open, so navigating does not also walk them
    // around. The Panorama surface freezes its own sessions the same way.
    Runtime.Freeze.Enable(true);
    if (const auto& menu = Settings.GetMenu(); menu.panorama)
    {
        Panorama.emplace(VoltMod::PanoramaMenu::Services{.Scheduler = Runtime.Scheduler,
                                                         .Slots = Runtime.Slots,
                                                         .Freeze = Runtime.Freeze,
                                                         .ChatInput = Runtime.Hooks.ChatInput,
                                                         .Translations = Runtime.Translations,
                                                         .Policy = Runtime.Policy,
                                                         .Screens = Runtime.Screens,
                                                         .Addons = Runtime.Addons},
                         MenuScreen, menu.addonId);
        PreferPanorama = Runtime.Menus.Prefer(*Panorama);
    }

    // Admins and punishments live in the database; without it their steps would only repeat its error.
    auto& steps = Runtime.LoadSteps;
    const bool database = steps.Optional("Database", [this] { return ConnectDatabase(); });
    if (database)
        steps.Optional("Admins", [this] { return LoadAdminData(); });

    RegisterCommands();

    if (database)
        steps.Optional("Punishments", [this] { return StartPunishments(); });

    RegisterGameEventListeners();
    // Queue fun-model assets; they replicate to clients from the next map load.
    Admin::Effects::PrecacheModels(Runtime);
    // Surface an unloadable configured map here rather than on the first !map.
    MapCycle.VerifyAgainstEngine();
    FunMode.Start();

    InstallStatusReporting();
    return true;
}

}  // namespace AdminSystem
