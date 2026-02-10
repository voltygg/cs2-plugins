#include "usermessage.h"

#include <ISmmPlugin.h>
#include <engine/igameeventsystem.h>
#include <networksystem/inetworkmessages.h>
#include <networksystem/netmessage.h>
#include <usermessages.pb.h>
#include <irecipientfilter.h>

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

namespace sdk {

IGameEventSystem* g_pGameEventSystem = nullptr;
INetworkMessages* g_pNetworkMessages = nullptr;
IGameEventManager2* g_pGameEventManager2 = nullptr;

bool InitMessageSystem()
{
    if (!g_pGameEventSystem)
    {
        META_CONPRINTF("[AdminSystem] Error: IGameEventSystem not available.\n");
        return false;
    }

    if (!g_pNetworkMessages)
    {
        META_CONPRINTF("[AdminSystem] Error: INetworkMessages not available.\n");
        return false;
    }

    // Note: g_pGameEventManager2 is resolved later via signature scan in InitEntitySystem()

    META_CONPRINTF("[AdminSystem] Message system initialized.\n");
    return true;
}

void SendCenterHtml(int slot, const std::string& html)
{
    if (!g_pGameEventManager2 || slot < 0 || slot >= 64)
        return;

    IGameEvent* pEvent = g_pGameEventManager2->CreateEvent("show_survival_respawn_status");
    if (!pEvent)
        return;

    pEvent->SetString("loc_token", html.c_str());
    pEvent->SetInt("userid", slot);
    pEvent->SetInt("duration", 1);
    g_pGameEventManager2->FireEvent(pEvent);
}

void SendChatMessage(int slot, const std::string& message)
{
    if (!g_pGameEventSystem || !g_pNetworkMessages || slot < 0 || slot >= 64)
        return;

    static INetworkMessageInternal* s_pSayText2Internal = nullptr;
    if (!s_pSayText2Internal)
    {
        s_pSayText2Internal = g_pNetworkMessages->FindNetworkMessageById(UM_SayText2);
        if (!s_pSayText2Internal)
            s_pSayText2Internal = g_pNetworkMessages->FindNetworkMessage("CUserMessageSayText2");
    }

    if (!s_pSayText2Internal)
        return;

    CNetMessage* pMsg = s_pSayText2Internal->AllocateMessage();
    if (!pMsg)
        return;

    auto* pSayText = pMsg->ToPB<CUserMessageSayText2>();
    pSayText->set_entityindex(-1);
    pSayText->set_chat(false);
    pSayText->set_messagename(message.c_str());

    uint64_t clients = (1ULL << slot);

    g_pGameEventSystem->PostEventAbstract(
        0, false, 1, &clients,
        s_pSayText2Internal, pMsg, 0,
        NetChannelBufType_t::BUF_RELIABLE
    );

    g_pNetworkMessages->DeallocateNetMessageAbstract(s_pSayText2Internal, pMsg);
}

void ClearCenterHtml(int slot)
{
    SendCenterHtml(slot, " ");
}

} // namespace sdk
