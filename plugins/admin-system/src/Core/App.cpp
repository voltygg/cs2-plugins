#include "Core/App.hpp"

#include "Admin/AdminMenu.hpp"
#include "Admin/Effects/Model.hpp"
#include "Commands/Commands.hpp"
#include "Config/ConfigManager.hpp"
#include "Punishments/KickNotice.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/App/PluginEntry.hpp>
#include <VoltMod/Database/Api.hpp>
#include <VoltMod/Events/EventTypes.hpp>
#include <VoltMod/Unsafe/Hook.hpp>
#include <string>
#include <utility>

using VoltMod::Error;
using VoltMod::Player;
using VoltMod::Status;
namespace Log = VoltMod::Log;

VOLTMOD_PLUGIN(AdminSystem::App);

namespace AdminSystem
{

App::~App()
{
    // Unpublish before destroying the managers that answer MetaFactory queries.
    AdminActions.Unpublish();
    AdminSection.Unpublish();
    CheatCheck.CancelAll();
    Effects.CancelAll();
    // Unload skips disconnect hooks; Clear() raises Players.Disconnected while its cleanup subscription is active.
    Runtime.Players.Clear();
    // Flush queued writes and discard undispatched completions before their managers are destroyed.
    Db.Disconnect();
}

void App::InstallPolicy()
{
    auto& policy = Runtime.Policy;
    policy.HasPermission = [this](int64_t steamId, std::string_view permission) {
        return Access.HasAnyPermission(steamId, std::string(permission));
    };
    // Policy::Authorize handles console and self-targeting; Policy::AuthorizeSteamId uses this for offline targets.
    policy.CanTarget = [this](int64_t callerSteamId, int64_t targetSteamId) {
        return Access.CanTarget(callerSteamId, targetSteamId);
    };
    policy.Reply = [this](int slot, std::string_view message) { Chat.Reply(slot, message); };
    policy.Broadcast = [this](const VoltMod::Authorized& who, std::string_view key) {
        // Target-less and self-targeted both read as "Bob noclipped"; one roster makes self pointer identity.
        const bool named = who.Target && who.Target != &who.Caller;
        Chat.BroadcastAction(key, who.Caller.Name(), named ? who.Target->Name() : std::string_view{});
    };
}

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

    // Register the language once so every slot-aware translation uses it.
    if (const auto* row = Admins.GetAdmin(steamId))
        Runtime.Translations.SetPlayerLanguage(slot, row->Language);

    // Notify frozen admins on connect instead of waiting for their first denied command.
    if (Freeze.IsFrozen(steamId))
        Freeze.NotifyFrozenSoon(slot, steamId);

    // Defer kicks because some builds cannot kick safely inside the connect hook; bots never match SteamID bans.
    if (auto ban = Punishments.GetActive(AdminSystem::Punishments::PunishType::Ban, steamId))
    {
        // Build the full notice before deferring because the ban row is only available here.
        Punishments.KickDeferred(
            slot, steamId,
            AdminSystem::Punishments::BuildBanNotice(Runtime.Translations, Settings.Get().punishments.appeal,
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
    if (!Db.Connect(Settings.Get().database))
        return std::unexpected(Error::Engine("unavailable; chat commands will reject all callers"));

    Migration = VoltMod::RunMigrations(Db, Runtime.PluginFile("configs/migrations"),
                                       {.HistoryTable = "schema_migrations", .LockKey = 727274});
    if (!Migration)
        return std::unexpected(Error::Failed("migrations failed; not loading admins against an out-of-date schema"));

    const auto& server = Settings.Get().server;
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

Status App::InitializePunishments()
{
    const bool loaded = Punishments.LoadActivePunishments();

    // Poll for cross-server freezes and keep this server's shared registry entry alive.
    _subs.Add(Runtime.Scheduler.Repeat(60'000, [this] {
        Punishments.ExpireOldPunishments();
        Freeze.RefreshFromDatabase();
        Repos.Servers.HeartbeatAsync(Settings.Get().server.tag);
    }));

    // Publish the anticheat surface only after its database and admin dependencies are ready.
    AdminActions.Publish();

    if (!loaded)
        return std::unexpected(Error::Failed("failed to load active punishments"));
    return {};
}

void App::RegisterVoiceMuteHook()
{
    _subs.Add(VoltMod::HookInterface(
        &IVEngineServer2::SetClientListening, Runtime.Unsafe.Interfaces.Engine,
        [this](IVEngineServer2& engine, CPlayerSlot receiver, CPlayerSlot sender,
               bool listen) -> VoltMod::HookResult<bool> {
            if (!listen)
                return {};
            VoltMod::Player* muted = Runtime.Players.Get(sender.Get());
            if (!muted || !Punishments.IsPunished(Punishments::PunishType::VoiceMute, muted->SteamId()))
                return {};

            // One hook call per receiver; ChatService rate-limits this to one chat line.
            PlayerChat.NotifyVoiceMuted(muted);
            // Run the engine's own handler with listening off instead of the caller's value.
            VoltMod::CallOriginal(&IVEngineServer2::SetClientListening, &engine, receiver, sender, false);
            return VoltMod::HookResult<bool>::Block(false);
        }));
}

void App::RegisterGameEventListeners()
{
    auto& events = Runtime.GameEvents;
    _subs.Add(events.On<VoltMod::PlayerDeath>([this](const VoltMod::PlayerDeath& e) {
        // Only per-life effects end on death; EffectScope::Session grants survive.
        if (e.VictimSlot >= 0)
            Effects.CancelOnDeath(e.VictimSlot);
    }));
    _subs.Add(events.On<VoltMod::RoundEnd>([this](const VoltMod::RoundEnd&) {
        Effects.CancelRound();
        // Apply queued map changes after the round so players can read the scoreboard.
        MapCycle.ChangeToNext();
    }));
    _subs.Add(events.On<VoltMod::RoundPrestart>([this](const VoltMod::RoundPrestart&) { Effects.CancelRound(); }));
}

void App::InstallStatusReporting()
{
    auto& status = Runtime.Status;

    status.RegisterSection("db", [this] {
        // Report current worker state so post-load failures and recoveries are visible.
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
        const auto& server = Settings.Get().server;
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

bool App::OpenAdminMenu(int slot)
{
    // Panel language is registered at connect (see OnPlayerConnect).
    auto menu = Admin::BuildAdminMainMenu(*this, slot);
    if (!menu)
        return false;

    Runtime.Menus.OpenSession(slot, std::move(menu), {});
    return true;
}

bool App::Load()
{
    if (!VoltMod::LoadStandardConfig(Runtime, Settings))
        return false;

    InstallPolicy();
    RegisterPlayerLifecycle();
    // Freeze players while menus are open so navigation input cannot also move them.
    Runtime.Freeze.Enable(true);
    if (const auto& menu = Settings.Get().menu; menu.panorama)
    {
        Panorama.emplace(VoltMod::PanoramaMenu::Services{.Scheduler = Runtime.Scheduler,
                                                         .Slots = Runtime.Slots,
                                                         .Freeze = Runtime.Freeze,
                                                         .ChatInput = Runtime.Hooks.ChatInput,
                                                         .Translations = Runtime.Translations,
                                                         .Policy = Runtime.Policy,
                                                         .Screens = Runtime.Screens,
                                                         .Addons = Runtime.Addons},
                         MenuLayout, menu.addonId);
        PreferPanorama = Runtime.Menus.Prefer(*Panorama);
    }

    // Skip database-dependent stages after a database failure.
    auto& steps = Runtime.LoadSteps;
    const bool database = steps.Optional("Database", [this] { return ConnectDatabase(); });
    if (database)
        steps.Optional("Admins", [this] { return LoadAdminData(); });

    RegisterCommands();
    AdminSection.Publish();

    if (database)
        steps.Optional("Punishments", [this] { return InitializePunishments(); });

    RegisterGameEventListeners();
    RegisterVoiceMuteHook();
    // Queued model assets reach clients on the next map load.
    Admin::Effects::PrecacheModels(Runtime);
    // Report invalid configured maps at load instead of on the first !map.
    MapCycle.VerifyAgainstEngine();
    FunMode.Initialize();

    InstallStatusReporting();
    return true;
}

}  // namespace AdminSystem
