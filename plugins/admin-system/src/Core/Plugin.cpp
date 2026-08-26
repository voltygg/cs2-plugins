#include "Plugin.hpp"

#include "App.hpp"
#include "Config.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/App/PluginInfoStamp.hpp>
#include <VoltMod/Runtime.hpp>
#include <VoltMod/Unsafe/HookMacros.hpp>
#include <string>

using VoltMod::Player;
using VoltMod::PluginInfo;
namespace Log = VoltMod::Log;

VOLTMOD_PLUGIN(AdminSystemPlugin);

SH_DECL_HOOK3(IVEngineServer2, SetClientListening, SH_NOATTRIB, 0, bool, CPlayerSlot, CPlayerSlot, bool);

PluginInfo AdminSystemPlugin::Info() const
{
    return VoltMod::WithBuildInfo({
        .Name = "Admin System",
        .Author = "Sukhrob Ilyosbekov",
        .Description = "Admin System for CS2",
        .Url = "https://github.com/voltygg/cs2-plugins",
        .LogTag = "ADMIN",
    });
}

bool AdminSystemPlugin::OnLoad(VoltMod::Runtime& runtime, bool /*late*/)
{
    Log::Info("Loading v{}...", Info().Version);
    _app.emplace(runtime, Info().Version);
    return _app->Start();
}

void AdminSystemPlugin::OnRegisterHooks(VoltMod::Runtime& runtime)
{
    _clientListening = VOLTMOD_SCOPED_HOOK(IVEngineServer2, SetClientListening, runtime.Interfaces.Engine,
                                           SH_MEMBER(this, &AdminSystemPlugin::Hook_SetClientListening), false);
}

bool AdminSystemPlugin::OnPlayerChat(Player* player, std::string_view message, bool teamChat)
{
    return _app->PlayerChat.HandleSay(player, message, teamChat);
}

bool AdminSystemPlugin::Hook_SetClientListening(CPlayerSlot iReceiver, CPlayerSlot iSender, bool bListen)
{
    if (bListen)
    {
        if (auto* sender = _app->Runtime.Players.Get(iSender.Get()))
        {
            if (_app->Punishments.IsVoiceMuted(sender->SteamId()))
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
