#pragma once

#include <string>
#include <igameevents.h>

// Forward declarations from HL2SDK
class IGameEventSystem;
class INetworkMessages;

namespace sdk {

// User message IDs (from usermessages.proto)
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

// Global interface pointers (set by InitMessageSystem / plugin Load)
extern IGameEventSystem* g_pGameEventSystem;
extern INetworkMessages* g_pNetworkMessages;
extern IGameEventManager2* g_pGameEventManager2;

} // namespace sdk
