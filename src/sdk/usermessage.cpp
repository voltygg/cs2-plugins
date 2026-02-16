#include "usermessage.h"
#include "sigscanner.h"

#include <ISmmPlugin.h>
#include <engine/igameeventsystem.h>
#include <networksystem/inetworkmessages.h>
#include <networksystem/netmessage.h>
#include <usermessages.pb.h>
#include <irecipientfilter.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

namespace sdk {

IGameEventSystem* g_pGameEventSystem = nullptr;
INetworkMessages* g_pNetworkMessages = nullptr;
IGameEventManager2* g_pGameEventManager2 = nullptr;

// GetLegacyGameEventListener function pointer — resolved via signature
using GetLegacyGameEventListenerFn = IGameEventListener2* (*)(CPlayerSlot slot);
static GetLegacyGameEventListenerFn s_pGetLegacyListener = nullptr;

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

    META_CONPRINTF("[AdminSystem] Message system initialized.\n");
    return true;
}

bool InitGameEventManager()
{
    // Load signatures from gamedata
    try
    {
        std::filesystem::path gamedata_path = std::filesystem::path(g_SMAPI->GetBaseDir())
            / "addons/admin-system/gamedata/admin_system.games.json";
        std::ifstream file(gamedata_path);
        if (!file.is_open())
        {
            META_CONPRINTF("[AdminSystem] Warning: gamedata file not found for signature scanning.\n");
            return false;
        }

        nlohmann::json gamedata = nlohmann::json::parse(file);

        if (!gamedata.contains("signatures"))
        {
            META_CONPRINTF("[AdminSystem] Warning: No signatures section in gamedata.\n");
            return false;
        }

        auto& sigs = gamedata["signatures"];

#ifdef _WIN32
        const char* platform = "windows";
        const char* offsetKey = "windows_offset";
#else
        const char* platform = "linux";
        const char* offsetKey = "linux_offset";
#endif

        // Resolve IGameEventManager2
        if (sigs.contains("CSource2Server::g_GameEventManager"))
        {
            auto& sigEntry = sigs["CSource2Server::g_GameEventManager"];
            if (sigEntry.contains(platform) && sigEntry.contains(offsetKey))
            {
                std::string pattern = sigEntry[platform].get<std::string>();
                std::string lib = sigEntry.value("lib", "server");
                int offset = sigEntry[offsetKey].get<int>();

                void* match = FindPattern(lib.c_str(), pattern);
                if (match)
                {
                    uintptr_t addr = reinterpret_cast<uintptr_t>(match) + offset;
                    addr = ResolveRelativeAddress(addr, 0, 4);
                    g_pGameEventManager2 = *reinterpret_cast<IGameEventManager2**>(addr);

                    if (g_pGameEventManager2)
                    {
                        META_CONPRINTF("[AdminSystem] Game event manager resolved at %p.\n",
                                       static_cast<void*>(g_pGameEventManager2));
                    }
                    else
                    {
                        META_CONPRINTF("[AdminSystem] Warning: Game event manager pointer is null after resolve.\n");
                    }
                }
                else
                {
                    META_CONPRINTF("[AdminSystem] Warning: GameEventManager signature not found.\n");
                }
            }
        }

        // Resolve GetLegacyGameEventListener (optional — used for per-player event delivery)
        if (sigs.contains("LegacyGameEventListener"))
        {
            auto& sigEntry = sigs["LegacyGameEventListener"];
            if (sigEntry.contains(platform))
            {
                std::string pattern = sigEntry[platform].get<std::string>();
                std::string lib = sigEntry.value("lib", "server");

                void* match = FindPattern(lib.c_str(), pattern);
                if (match)
                {
                    s_pGetLegacyListener = reinterpret_cast<GetLegacyGameEventListenerFn>(match);
                    META_CONPRINTF("[AdminSystem] LegacyGameEventListener resolved.\n");
                }
                else
                {
                    META_CONPRINTF("[AdminSystem] Warning: LegacyGameEventListener signature not found (will use broadcast fallback).\n");
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        META_CONPRINTF("[AdminSystem] Warning: Failed to parse gamedata for signatures: %s\n", e.what());
        return false;
    }

    return g_pGameEventManager2 != nullptr;
}

// Fresh event each frame, matching cs2-menus pattern exactly:
// CreateEvent → SetFields → FireGameEvent → FreeEvent, every frame, duration=5.

void SendCenterHtml(int slot, const std::string& html)
{
    if (!g_pGameEventManager2 || slot < 0 || slot >= 64)
        return;

    IGameEvent* pEvent = g_pGameEventManager2->CreateEvent("show_survival_respawn_status");
    if (!pEvent)
        return;

    pEvent->SetString("loc_token", html.c_str());
    pEvent->SetInt("userid", slot);
    pEvent->SetInt("duration", 5);

    if (s_pGetLegacyListener)
    {
        IGameEventListener2* pListener = s_pGetLegacyListener(CPlayerSlot(slot));
        if (pListener)
        {
            pListener->FireGameEvent(pEvent);
            g_pGameEventManager2->FreeEvent(pEvent);
            return;
        }
    }

    // Fallback: broadcast (FireEvent frees the event)
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
