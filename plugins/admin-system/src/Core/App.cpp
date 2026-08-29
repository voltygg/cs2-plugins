#include "App.hpp"

#include "../Admin/Effects/Model.hpp"
#include "../Commands/Commands.hpp"
#include "../Database/Repositories/ServerRepository.hpp"
#include "../Punishments/KickNotice.hpp"
#include "Config.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Database/Api.hpp>
#include <VoltMod/Events/EventTypes.hpp>
#include <nlohmann/json.hpp>
#include <string>

using VoltMod::Player;
using VoltMod::StageResult;
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
    // caller targeting themselves before this is consulted.
    policy.CanTarget = [this](const Player& caller, const Player& target) {
        return Access.CanTarget(caller.SteamId(), target.SteamId());
    };
    policy.Reply = [this](int slot, std::string_view message) { Chat.Reply(slot, message); };
    policy.Broadcast = [this](const VoltMod::Authorized& who, std::string_view key) {
        if (who.Target)
            Chat.BroadcastAction(std::string(key), who.Caller.Name(), who.Target->Name());
    };
}

// The connection lifecycle: one subscription per edge, kept in _subs so the handlers stop before
// the managers they touch are destroyed.
void App::RegisterPlayerLifecycle()
{
    _subs.On(Runtime.Players.Connected, [this](Player& player) { OnPlayerConnect(player); });
    _subs.On(Runtime.Players.Disconnected, [this](Player& player) { OnPlayerDisconnect(player); });
}

void App::OnPlayerConnect(Player& player)
{
    const int64_t steamId = player.SteamId();
    const int slot = player.Slot();
    PlayerRepo.RecordConnect(steamId, player.Name(), std::string(player.Ip()));

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
    if (auto ban = Punishments.GetActiveBan(steamId))
    {
        // Built now, while the ban row is in hand, so the disconnect screen carries the expiry
        // and appeal link rather than the bare reason.
        Punishments.KickDeferred(slot, steamId,
                                 AdminSystem::Punishments::BuildBanNotice(Runtime.Translations, Config.GetAppeal(),
                                                                          ban->Reason, ban->ExpiresAt, steamId, slot));
    }
}

void App::OnPlayerDisconnect(Player& player)
{
    PlayerRepo.RecordDisconnect(player.SteamId(), player.Name(), player.Playtime().count());
    Effects.CancelAll(player.Slot());
    CheatCheck.CancelAllForSlot(player.Slot());
}

StageResult App::ConnectDatabase()
{
    if (!Db.Start(Config.GetDatabase()))
        return StageResult::Degraded("unavailable; chat commands will reject all callers");

    Migration = VoltMod::RunMigrations(Db, VoltMod::AddonFile(Core::AddonName, "configs/migrations"),
                                       {.TableName = "schema_migrations", .AdvisoryLockKey = 727274});
    if (!Migration)
        return StageResult::Degraded("migrations failed; not loading admins against an out-of-date schema");

    const auto& server = Config.GetServer();
    if (!Database::ServerRepository{Db}.Upsert(server.tag, server.name))
        Log::Warn("Failed to register server '{}' in the servers table.", server.tag);

    return StageResult::Ok();
}

StageResult App::LoadAdminData()
{
    const bool groups = Admins.LoadGroups();
    const bool admins = Admins.LoadAdmins();
    Freeze.RefreshFromDatabase();
    if (!groups || !admins)
        return StageResult::Degraded("failed to load groups/admins from DB");
    return StageResult::Ok();
}

StageResult App::StartPunishments()
{
    const bool loaded = Punishments.LoadActivePunishments();

    // Every minute: sweep expired bans/mutes, pick up admin freezes issued on other servers
    // sharing this database, and advance this server's registry heartbeat.
    _subs.Add(Runtime.Scheduler.Repeat(60'000, [this] {
        Punishments.ExpireOldPunishments();
        Freeze.RefreshFromDatabase();
        Database::ServerRepository{Db}.Heartbeat(Config.GetServer().tag);
    }));

    // Typed surface the anticheat plugin drives (bans need the DB, alerts need admin data).
    // Published last in this stage so a peer never sees a half-initialised implementation.
    AdminActions.Publish();

    return loaded ? StageResult::Ok() : StageResult::Degraded("failed to load active punishments");
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
        return nlohmann::json{{"connected", Db.IsConnected()},
                              {"migrationVersion", Migration.CurrentVersion},
                              {"migrationsApplied", Migration.Applied}}
            .dump();
    });

    status.RegisterSection("admins", [this] {
        return nlohmann::json{{"cached", Admins.AdminCount()}, {"groups", Admins.GroupCount()}}.dump();
    });

    status.RegisterSection("commands",
                           [this] { return nlohmann::json{{"registered", Runtime.Commands.Count()}}.dump(); });

    status.RegisterSection("server", [this] {
        const auto& server = Config.GetServer();
        return nlohmann::json{{"tag", server.tag}, {"name", server.name}}.dump();
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
    auto& report = Runtime.LoadReport;

    if (!VoltMod::LoadStandardConfig(Runtime, Config, {.Addon = Core::AddonName}))
        return false;

    report.Run("Policy", [this] {
        InstallPolicy();
        RegisterPlayerLifecycle();
        SelectMenuDriver();
        // Freeze the player while an admin menu is open. Center HTML needs it so WASD navigation
        // doesn't also walk them around; the Panorama menu needs it because a cursor takes
        // mouse-look and being shoved around while clicking is worse, not better.
        Runtime.Menus.FreezeWhileOpen(true);
        return StageResult::Ok();
    });

    report.Run("Database", [this] { return ConnectDatabase(); });

    report.Run("Admins", [&] {
        if (!report.IsOk("Database"))
            return StageResult::Skipped("database unavailable");
        return LoadAdminData();
    });

    report.Run("Commands", [this] {
        RegisterCommands();
        return StageResult::Ok(std::format("{} chat commands", Runtime.Commands.Count()));
    });

    report.Run("Punishments", [&] {
        if (!report.IsOk("Database"))
            return StageResult::Skipped("database unavailable");
        return StartPunishments();
    });

    report.Run("Events", [this] {
        RegisterGameEventListeners();
        // Queue fun-model assets; they replicate to clients from the next map load.
        Admin::Effects::PrecacheModels(Runtime);
        // Surface an unloadable configured map here rather than on the first !map.
        MapCycle.VerifyAgainstEngine();
        FunMode.Start();
        return StageResult::Ok();
    });

    InstallStatusReporting();
    return true;
}

void App::SelectMenuDriver()
{
    const auto& settings = Config.GetMenu();

    // Worth saying out loud either way: a server configured for the styled menu and silently
    // serving the plain one otherwise looks like the setting was ignored.
    if (Config.GetMenuStyle() == Core::MenuStyle::CenterHtml)
    {
        VoltMod::Log::Info("Menus: center HTML, as configured.");
        return;
    }

    if (auto chosen = Runtime.Menus.UsePanorama(settings.layout); !chosen)
    {
        VoltMod::Log::Info("Menus: center HTML - {}.", chosen.error().Detail);
        return;
    }

    // A workshop addon is how the layout reaches players who did not copy it in by hand. The
    // requirement lasts exactly as long as this lease, so it goes when the plugin does.
    if (settings.addonId != 0)
    {
        if (auto lease = Runtime.Addons.Require(settings.addonId))
            _menuAddon = std::move(*lease);
        else
            VoltMod::Log::Warn("Menu addon {} not required ({}); players without the layout see nothing.",
                               settings.addonId, lease.error().Detail);
    }

    VoltMod::Log::Info("Menus: Panorama, layout '{}'.", settings.layout);
}

}  // namespace AdminSystem
