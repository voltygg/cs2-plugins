#include "App.hpp"

#include "../Admin/Effects/Model.hpp"
#include "../Database/Repositories/ServerRepository.hpp"
#include "Config.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Database/Api.hpp>
#include <nlohmann/json.hpp>
#include <string>

using CS2Kit::Player;
using CS2Kit::StageResult;
using CS2Kit::StageStatus;
namespace Log = CS2Kit::Log;

namespace AdminSystem
{

namespace Commands
{
void RegisterAdminMenuCommand(CS2Kit::CommandManager& commands, App& app);
void RegisterAdminSelfCommands(CS2Kit::CommandManager& commands, App& app);
void RegisterCheatCheckCommands(CS2Kit::CommandManager& commands, App& app);
void RegisterFreezeCommands(CS2Kit::CommandManager& commands, App& app);
void RegisterInfoCommands(CS2Kit::CommandManager& commands, App& app);
void RegisterPunishmentCommands(CS2Kit::CommandManager& commands, App& app);
void RegisterReportCommand(CS2Kit::CommandManager& commands, App& app);
}  // namespace Commands

App::~App()
{
    // Stop answering other plugins' MetaFactory queries before the managers it delegates to go.
    AdminActions.Unpublish();
    CheatCheck.CancelAll();
    Effects.CancelAll();
    // Unload fires no disconnect hooks, so fold open sessions here or lose their playtime.
    for (auto* player : Runtime.Players.GetAllPlayers())
        FlushPlayerSession(player);
    Runtime.Players.Clear();
    // Drains queued writes (a ban issued just before unload must land) and drops undispatched
    // completions before the managers they would touch are destroyed.
    Db.Stop();
}

void App::FlushPlayerSession(Player* player)
{
    if (player)
        PlayerRepo.RecordDisconnect(player->GetSteamID(), player->GetName(), player->GetPlaytime());
}

// The one policy the kit consults everywhere: command permissions, action targeting, result
// replies, and action broadcasts.
void App::InstallPolicy()
{
    Runtime.Policy = {
        .HasPermission = [this](int64_t steamId,
                                const std::string& permission) { return Access.HasAnyPermission(steamId, permission); },
        .CanTarget = [this](Player& caller,
                            Player& target) { return Access.CanTarget(caller.GetSteamID(), target.GetSteamID()); },
        .Reply = [this](int slot, std::string_view message) { Chat.Reply(slot, message); },
        .Broadcast =
            [this](Player& caller, Player* target, const std::string& key) {
                if (target)
                    Chat.BroadcastAction(key, caller.GetName(), target->GetName());
            },
    };
}

StageResult App::ConnectDatabase()
{
    if (!Db.Start(Config.GetDatabase()))
        return StageResult::Degraded("unavailable; chat commands will reject all callers");

    Migration = CS2Kit::RunMigrations(Db, CS2Kit::AddonFile(Core::AddonName, "configs/migrations"),
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
    _maintenance = Runtime.Scheduler.Repeat(60'000, [this] {
        Punishments.ExpireOldPunishments();
        Freeze.RefreshFromDatabase();
        Database::ServerRepository{Db}.Heartbeat(Config.GetServer().tag);
    });

    // Typed surface the anticheat plugin drives (bans need the DB, alerts need admin data).
    // Published last in this stage so a peer never sees a half-initialised implementation.
    AdminActions.Publish();

    return loaded ? StageResult::Ok() : StageResult::Degraded("failed to load active punishments");
}

void App::RegisterGameEventListeners()
{
    namespace Events = CS2Kit::Events;
    auto& events = Runtime.Events;
    _playerDeath = events.Listen<Events::PlayerDeath>([this](const Events::PlayerDeath& e) {
        // Clear per-life effects; EffectScope::Session grants (e.g. bhop) survive death.
        if (e.VictimSlot >= 0)
            Effects.CancelPerLife(e.VictimSlot);
    });
    _roundEnd = events.Listen<Events::RoundEnd>([this](const Events::RoundEnd&) { Effects.CancelRoundScoped(); });
    _roundPrestart =
        events.Listen<Events::RoundPrestart>([this](const Events::RoundPrestart&) { Effects.CancelRoundScoped(); });
}

// Domain sections on top of the kit's (build/load/gamedata/uptime), plus the command that
// reports them. Health adds the database to the kit's baseline: an admin plugin that cannot
// reach its database is not healthy even though the load itself succeeded.
void App::InstallStatusReporting()
{
    auto& status = Runtime.Status;

    status.RegisterSection("db", [this] {
        // Live worker state, not the load-time stage result: a database that died (or recovered)
        // after load must show as such.
        return nlohmann::json{{"connected", Db.IsConnected()},
                              {"migrationVersion", Migration.CurrentVersion},
                              {"migrationsApplied", Migration.Applied}};
    });

    status.RegisterSection(
        "admins", [this] { return nlohmann::json{{"cached", Admins.AdminCount()}, {"groups", Admins.GroupCount()}}; });

    status.RegisterSection("commands", [this] { return nlohmann::json{{"registered", Runtime.Commands.Count()}}; });

    status.RegisterSection("server", [this] {
        const auto& server = Config.GetServer();
        return nlohmann::json{{"tag", server.tag}, {"name", server.name}};
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

    // "Configuration" + "Translations" stages, via ConfigManager::LoadSettings.
    if (!CS2Kit::LoadStandardConfig(Runtime, Config, {.Addon = Core::AddonName}))
        return false;

    report.Run("Policy", [this] {
        InstallPolicy();
        // Freeze the player while an admin menu is open so WASD navigation doesn't also walk
        // them around.
        Runtime.Menus.SetFreezePlayer(true);
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
        return StageResult::Ok();
    });

    InstallStatusReporting();
    return true;
}

}  // namespace AdminSystem
