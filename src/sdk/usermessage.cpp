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

static INetworkMessageInternal* s_pTextMsgInternal = nullptr;

// Lazy-resolve a network message (not available during Load(), only after server startup)
static INetworkMessageInternal* ResolveTextMsg()
{
    META_CONPRINTF("[AdminSystem] Resolving TextMsg network message...\n");

    if (s_pTextMsgInternal)
    {
        META_CONPRINTF("[AdminSystem] TextMsg already resolved.\n");
        return s_pTextMsgInternal;
    }


    if (!g_pNetworkMessages)
    {
        META_CONPRINTF("[AdminSystem] Error: INetworkMessages not available.\n");
        return nullptr;
    }

    // Try by ID first, then by name
    s_pTextMsgInternal = g_pNetworkMessages->FindNetworkMessageById(UM_TextMsg);
    if (!s_pTextMsgInternal)
        s_pTextMsgInternal = g_pNetworkMessages->FindNetworkMessage("CUserMessageTextMsg");

    if (s_pTextMsgInternal)
        META_CONPRINTF("[AdminSystem] TextMsg network message resolved.\n");

    return s_pTextMsgInternal;
}

bool InitMessageSystem()
{
    // Interfaces are obtained via GET_V_IFACE_ANY in plugin Load()
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

    // Network messages are registered after entity networking is built,
    // which happens after plugin Load(). TextMsg will be resolved lazily on first use.
    META_CONPRINTF("[AdminSystem] Message system initialized (messages resolve on first use).\n");
    return true;
}

void SendCenterHtml(int slot, const std::string& html)
{
    if (!g_pGameEventSystem || slot < 0 || slot >= 64)
        return;

    auto* pTextMsgInternal = ResolveTextMsg();
    if (!pTextMsgInternal)
        return;

    CNetMessage* pMsg = pTextMsgInternal->AllocateMessage();
    if (!pMsg)
        return;

    auto* pTextMsg = pMsg->ToPB<CUserMessageTextMsg>();
    pTextMsg->set_dest(HUD_PRINTCENTER);
    pTextMsg->add_param(html.c_str());

    // Build client bitmask: bit position corresponds to client slot
    uint64_t clients = (1ULL << slot);

    g_pGameEventSystem->PostEventAbstract(
        0,           // CSplitScreenSlot
        false,       // bLocalOnly
        1,           // nClientCount
        &clients,    // client bitmask
        pTextMsgInternal,
        pMsg,
        0,           // nSize (unused)
        NetChannelBufType_t::BUF_RELIABLE
    );

    g_pNetworkMessages->DeallocateNetMessageAbstract(pTextMsgInternal, pMsg);
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
