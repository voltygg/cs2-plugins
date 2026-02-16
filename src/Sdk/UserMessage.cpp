#include "UserMessage.hpp"
#include "SigScanner.hpp"
#include "GameData.hpp"
#include "GameInterfaces.hpp"
#include "../Utils/Log.hpp"

#include <ISmmPlugin.h>
#include <igameevents.h>
#include <engine/igameeventsystem.h>
#include <networksystem/inetworkmessages.h>
#include <networksystem/netmessage.h>
#include <usermessages.pb.h>
#include <irecipientfilter.h>

using namespace AdminSystem::Utils;

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

namespace AdminSystem::Sdk {

// GetLegacyGameEventListener function pointer - resolved via signature
using GetLegacyGameEventListenerFn = IGameEventListener2* (*)(CPlayerSlot slot);
static GetLegacyGameEventListenerFn sGetLegacyListener = nullptr;

bool InitMessageSystem()
{
    auto& interfaces = GameInterfaces::Instance();

    if (!interfaces.GameEventSystem)
    {
        Log::Error("IGameEventSystem not available.");
        return false;
    }

    if (!interfaces.NetworkMessages)
    {
        Log::Error("INetworkMessages not available.");
        return false;
    }

    Log::Info("Message system initialized.");
    return true;
}

bool InitGameEventManager()
{
    auto& interfaces = GameInterfaces::Instance();
    auto& gameData = GameData::Instance();

    // Resolve IGameEventManager2 via GameData signature
    void* eventManagerAddr = gameData.ResolveSignature("CSource2Server::g_GameEventManager");
    if (eventManagerAddr)
    {
        interfaces.GameEventManager = *reinterpret_cast<IGameEventManager2**>(
            reinterpret_cast<uintptr_t>(eventManagerAddr));

        if (interfaces.GameEventManager)
        {
            Log::Info("Game event manager resolved at {:#x}.",
                             reinterpret_cast<uintptr_t>(interfaces.GameEventManager));
        }
        else
        {
            Log::Warn("Game event manager pointer is null after resolve.");
        }
    }
    else
    {
        Log::Warn("GameEventManager signature not found.");
    }

    // Resolve GetLegacyGameEventListener (optional - used for per-player event delivery)
    void* legacyListenerAddr = gameData.FindSignature("LegacyGameEventListener");
    if (legacyListenerAddr)
    {
        sGetLegacyListener = reinterpret_cast<GetLegacyGameEventListenerFn>(legacyListenerAddr);
        Log::Info("LegacyGameEventListener resolved.");
    }
    else
    {
        Log::Warn("LegacyGameEventListener signature not found (will use broadcast fallback).");
    }

    return interfaces.GameEventManager != nullptr;
}

// Fresh event each frame, matching cs2-menus pattern exactly:
// CreateEvent -> SetFields -> FireGameEvent -> FreeEvent, every frame, duration=5.

void SendCenterHtml(int slot, const std::string& html)
{
    auto* gameEventManager = GameInterfaces::Instance().GameEventManager;
    if (!gameEventManager || slot < 0 || slot >= 64)
        return;

    IGameEvent* pEvent = gameEventManager->CreateEvent("show_survival_respawn_status");
    if (!pEvent)
        return;

    pEvent->SetString("loc_token", html.c_str());
    pEvent->SetInt("userid", slot);
    pEvent->SetInt("duration", 5);

    if (sGetLegacyListener)
    {
        IGameEventListener2* pListener = sGetLegacyListener(CPlayerSlot(slot));
        if (pListener)
        {
            pListener->FireGameEvent(pEvent);
            gameEventManager->FreeEvent(pEvent);
            return;
        }
    }

    // Fallback: broadcast (FireEvent frees the event)
    gameEventManager->FireEvent(pEvent);
}

void SendChatMessage(int slot, const std::string& message)
{
    auto& interfaces = GameInterfaces::Instance();
    if (!interfaces.GameEventSystem || !interfaces.NetworkMessages || slot < 0 || slot >= 64)
        return;

    static INetworkMessageInternal* sSayText2Internal = nullptr;
    if (!sSayText2Internal)
    {
        sSayText2Internal = interfaces.NetworkMessages->FindNetworkMessageById(UmSayText2);
        if (!sSayText2Internal)
            sSayText2Internal = interfaces.NetworkMessages->FindNetworkMessage("CUserMessageSayText2");
    }

    if (!sSayText2Internal)
        return;

    CNetMessage* pMsg = sSayText2Internal->AllocateMessage();
    if (!pMsg)
        return;

    auto* pSayText = pMsg->ToPB<CUserMessageSayText2>();
    if (!pSayText)
    {
        interfaces.NetworkMessages->DeallocateNetMessageAbstract(sSayText2Internal, pMsg);
        return;
    }
    pSayText->set_entityindex(-1);
    pSayText->set_chat(false);
    pSayText->set_messagename(message.c_str());

    uint64_t clients = (1ULL << slot);

    interfaces.GameEventSystem->PostEventAbstract(
        0, false, 1, &clients,
        sSayText2Internal, pMsg, 0,
        NetChannelBufType_t::BUF_RELIABLE
    );

    interfaces.NetworkMessages->DeallocateNetMessageAbstract(sSayText2Internal, pMsg);
}

void ClearCenterHtml(int slot)
{
    SendCenterHtml(slot, " ");
}

} // namespace AdminSystem::Sdk
