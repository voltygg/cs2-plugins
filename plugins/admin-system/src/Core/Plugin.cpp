#include "Plugin.hpp"

#include "App.hpp"
#include "Config.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/App/PluginInfoStamp.hpp>
#include <CS2Kit/Core/HookMacros.hpp>
#include <CS2Kit/Runtime.hpp>
#include <string>

using CS2Kit::Player;
using CS2Kit::PluginInfo;
using CS2Kit::Sdk::PlayerController;
namespace Log = CS2Kit::Log;

CS2KIT_PLUGIN(AdminSystemPlugin);

SH_DECL_HOOK3(IVEngineServer2, SetClientListening, SH_NOATTRIB, 0, bool, CPlayerSlot, CPlayerSlot, bool);

PluginInfo AdminSystemPlugin::Info() const
{
    return CS2Kit::WithBuildInfo({
        .Name = "Admin System",
        .Author = "Sukhrob Ilyosbekov",
        .Description = "Admin System for CS2",
        .Url = "https://github.com/voltygg/cs2-plugins",
        .LogTag = "ADMIN",
    });
}

bool AdminSystemPlugin::OnLoad(CS2Kit::Runtime& runtime, bool /*late*/)
{
    Log::Info("Loading v{}...", Info().Version);
    _app.emplace(runtime, Info().Version);
    return _app->Start();
}

void AdminSystemPlugin::OnRegisterHooks(CS2Kit::Runtime& runtime)
{
    _clientListening = CS2KIT_SCOPED_HOOK(IVEngineServer2, SetClientListening, runtime.Interfaces.Engine,
                                          SH_MEMBER(this, &AdminSystemPlugin::Hook_SetClientListening), false);
}

void AdminSystemPlugin::OnPlayerConnect(Player* player)
{
    if (!player)
        return;

    auto& app = *_app;
    app.PlayerRepo.RecordConnect(player->GetSteamID(), player->GetName(), player->GetIpAddress());

    // Register the admin's panel language up front so every slot-aware Translations::Get (menus,
    // cheat-check, mute notices) renders in their language without per-command setup.
    if (const auto* row = app.Admins.GetAdmin(player->GetSteamID()))
        app.Runtime.Translations.SetPlayerLanguage(player->GetSlot(), row->Language);

    // A frozen admin gets told up front instead of discovering it on their first denied command.
    // Deferred a tick like the ban kick below so the freshly-connected client receives the line.
    if (app.Freeze.IsFrozen(player->GetSteamID()))
    {
        int64_t steamId = player->GetSteamID();
        app.Runtime.Scheduler.NextTick([&app, steamId] { app.Freeze.NotifyFrozen(steamId); });
    }

    // Reject banned players. Kicking inside the connect hook is unsafe in some builds, so we defer
    // to the next game frame via the scheduler -- the player is fully connected by then. Bots have
    // no real SteamID and never match an active ban.
    if (auto ban = app.Punishments.GetActiveBan(player->GetSteamID()))
    {
        int slot = player->GetSlot();
        std::string reason = ban->Reason;
        app.Runtime.Scheduler.NextTick([slot, reason] { PlayerController(slot).Kick(reason.c_str()); });
    }
}

void AdminSystemPlugin::OnPlayerDisconnect(Player* player)
{
    if (!player)
        return;

    _app->FlushPlayerSession(player);
    _app->Effects.CancelAllForSlot(player->GetSlot());
    _app->CheatCheck.CancelAllForSlot(player->GetSlot());
}

bool AdminSystemPlugin::OnPlayerChat(Player* player, std::string_view message, bool teamChat)
{
    return _app->PlayerChat.HandleSay(player, message, teamChat);
}

bool AdminSystemPlugin::Hook_SetClientListening(CPlayerSlot iReceiver, CPlayerSlot iSender, bool bListen)
{
    if (bListen)
    {
        if (auto* sender = _app->Runtime.Players.GetPlayerBySlot(iSender.Get()))
        {
            if (_app->Punishments.IsVoiceMuted(sender->GetSteamID()))
            {
                // Tell the muted player they're being suppressed; ChatService rate-limits this
                // so the per-receiver explosion of hook calls collapses to one chat line.
                _app->PlayerChat.NotifyVoiceMuted(sender);
                RETURN_META_VALUE_NEWPARAMS(MRES_HANDLED, false, &IVEngineServer2::SetClientListening,
                                            (iReceiver, iSender, false));
            }
        }
    }
    RETURN_META_VALUE(MRES_IGNORED, true);
}
