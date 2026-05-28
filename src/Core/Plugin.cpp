#include "Plugin.hpp"

#include "../Admin/AdminManager.hpp"
#include "../Admin/Effects/EffectManager.hpp"
#include "../Commands/AdminCommands.hpp"
#include "../Database/Database.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "ChatService.hpp"
#include "Config.hpp"

#include <CS2Kit/Commands/CommandManager.hpp>
#include <CS2Kit/Core/Scheduler.hpp>
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
using AdminSystem::Database::Database;

AdminSystemPlugin g_AdminSystemPlugin;
PLUGIN_EXPOSE(AdminSystemPlugin, g_AdminSystemPlugin);

SH_DECL_HOOK3(IVEngineServer2, SetClientListening, SH_NOATTRIB, 0, bool, CPlayerSlot, CPlayerSlot, bool);

// ------- Subsystem wiring -----

namespace
{

bool LoadConfigs()
{
    if (!ConfigManager::Instance().LoadSettings("addons/admin-system/configs/settings.json"))
    {
        Log::Error("Failed to load settings.json -- aborting load.");
        return false;
    }
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

    if (!db.Initialize(ConfigManager::Instance().GetDatabase()))
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

    Log::Info("Loading configurations...");
    if (!LoadConfigs())
        return false;

    auto locale = ConfigManager::Instance().GetPlugin().locale;
    Log::Info("Translations: Setting language to {}...", locale);
    Translations::Instance().SetLanguage(locale);

    InstallCommandCallbacks();

    bool dbConnected = ConnectDatabaseAndLoadAdmins();
    if (dbConnected)
        Defer([] { Database::Instance().CloseConnection(); });

    Log::Info("Initializing commands...");
    AdminSystem::Commands::RegisterAdminCommands(CommandManager::Instance());

    if (dbConnected)
        RegisterPunishmentTasks();

    Log::Info("Loading translations...");
    Translations::Instance().Load("addons/admin-system/configs/translations");

    RegisterGameEventListeners();

    Defer([] { EffectManager::Instance().CancelAll(); });
    Defer([] { PlayerManager::Instance().Clear(); });

    Log::Info("All subsystems initialized.");
    return true;
}

void AdminSystemPlugin::OnPlayerConnect(Player* player)
{
    if (!player)
        return;

    // Reject banned players. Kicking inside the connect hook is unsafe in some builds, so we defer
    // to the next game frame via the scheduler -- the player is fully connected by then. Bots have
    // no real SteamID and never match an active ban.
    if (auto ban = PunishmentManager::Instance().GetActiveBan(player->GetSteamID()))
    {
        int slot = player->GetSlot();
        std::string reason = ban->Reason;
        Scheduler::Instance().NextTick([slot, reason]() { PlayerController(slot).Kick(reason.c_str()); });
    }
}

void AdminSystemPlugin::OnPlayerDisconnect(Player* player)
{
    if (player)
        EffectManager::Instance().CancelAllForSlot(player->GetSlot());
}

bool AdminSystemPlugin::OnPlayerChat(Player* player, std::string_view message, bool teamChat)
{
    return ChatService::Instance().HandleSay(player, std::string(message), teamChat);
}

void AdminSystemPlugin::OnRegisterHooks()
{
    auto& gi = GameInterfaces::Instance();
    SH_ADD_HOOK(IVEngineServer2, SetClientListening, gi.Engine,
                SH_MEMBER(this, &AdminSystemPlugin::Hook_SetClientListening), false);

    Defer([this] {
        auto& g = GameInterfaces::Instance();
        SH_REMOVE_HOOK(IVEngineServer2, SetClientListening, g.Engine,
                       SH_MEMBER(this, &AdminSystemPlugin::Hook_SetClientListening), false);
    });
}

bool AdminSystemPlugin::Hook_SetClientListening(CPlayerSlot iReceiver, CPlayerSlot iSender, bool bListen)
{
    if (bListen)
    {
        if (auto* sender = PlayerManager::Instance().GetPlayerBySlot(iSender.Get()))
        {
            if (PunishmentManager::Instance().IsVoiceMuted(sender->GetSteamID()))
            {
                // Tell the muted player they're being suppressed; ChatService rate-limits this
                // so the per-receiver explosion of hook calls collapses to one chat line.
                ChatService::Instance().NotifyVoiceMuted(sender);
                RETURN_META_VALUE_NEWPARAMS(MRES_HANDLED, false, &IVEngineServer2::SetClientListening,
                                            (iReceiver, iSender, false));
            }
        }
    }
    RETURN_META_VALUE(MRES_IGNORED, true);
}
