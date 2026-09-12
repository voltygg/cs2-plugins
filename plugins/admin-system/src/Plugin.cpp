#include "Plugin.hpp"

#include "Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/App/PluginInfoStamp.hpp>
#include <VoltMod/Runtime.hpp>
#include <VoltMod/Unsafe/Hook.hpp>
#include <string>

using VoltMod::Player;
using VoltMod::PluginInfo;
namespace Log = VoltMod::Log;

VOLTMOD_PLUGIN(AdminSystemPlugin);

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

bool AdminSystemPlugin::OnLoad(VoltMod::Runtime& runtime)
{
    Log::Info("Loading v{}...", Info().Version);
    _app.emplace(runtime, Info().Version);
    return _app->Start();
}

void AdminSystemPlugin::OnRegisterHooks(VoltMod::Runtime& runtime, VoltMod::Subscriptions& hooks)
{
    hooks.Add(VoltMod::HookInterface(&IVEngineServer2::SetClientListening, runtime.Unsafe.Interfaces.Engine,
                                     [this](IVEngineServer2& engine, CPlayerSlot receiver, CPlayerSlot sender,
                                            bool listen) {
                                         return OnSetClientListening(engine, receiver, sender, listen);
                                     }));
}

bool AdminSystemPlugin::OnPlayerChat(Player* player, std::string_view message, bool teamChat)
{
    return _app->PlayerChat.HandleSay(player, message, teamChat);
}

VoltMod::HookResult<bool> AdminSystemPlugin::OnSetClientListening(IVEngineServer2& engine, CPlayerSlot receiver,
                                                                  CPlayerSlot sender, bool listen)
{
    if (listen)
    {
        if (auto* muted = _app->Runtime.Players.Get(sender.Get()))
        {
            if (_app->Punishments.IsPunished(AdminSystem::Punishments::PunishType::VoiceMute, muted->SteamId()))
            {
                // Tell the muted player they're being suppressed; ChatService rate-limits this
                // so the per-receiver explosion of hook calls collapses to one chat line.
                _app->PlayerChat.NotifyVoiceMuted(muted);
                // Run the engine's own handler with listening off instead of the caller's value.
                VoltMod::CallOriginal(&IVEngineServer2::SetClientListening, &engine, receiver, sender, false);
                return VoltMod::HookResult<bool>::Block(false);
            }
        }
    }
    return {};
}
