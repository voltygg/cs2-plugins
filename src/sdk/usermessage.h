#pragma once

#include <string>

// Forward declarations from HL2SDK
class IGameEventSystem;
class INetworkMessages;
class INetworkMessageInternal;
class CNetMessage;

namespace sdk {

// HUD message destinations
constexpr int HUD_PRINTNOTIFY = 1;
constexpr int HUD_PRINTCONSOLE = 2;
constexpr int HUD_PRINTTALK = 3;
constexpr int HUD_PRINTCENTER = 4;

// Base user message IDs (from usermessages.proto)
constexpr int UM_TextMsg = 124;
constexpr int UM_SayText2 = 118;

/**
 * @brief Initialize SDK message interfaces. Call during plugin Load().
 * @return true if interfaces obtained successfully
 */
bool InitMessageSystem();

/**
 * @brief Send center HTML text to a specific player slot.
 */
void SendCenterHtml(int slot, const std::string& html);

/**
 * @brief Send a chat message to a specific player slot.
 */
void SendChatMessage(int slot, const std::string& message);

/**
 * @brief Clear center HTML for a specific player slot.
 */
void ClearCenterHtml(int slot);

// Global interface pointers (set by InitMessageSystem)
extern IGameEventSystem* g_pGameEventSystem;
extern INetworkMessages* g_pNetworkMessages;

} // namespace sdk
