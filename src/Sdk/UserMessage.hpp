#pragma once

#include <string>

namespace AdminSystem::Sdk
{

/** Network message ID for the SayText2 user message. */
constexpr int UmSayText2 = 118;

/** Initialize the network message system (resolves SayText2 message object). */
bool InitMessageSystem();
/** Resolve IGameEventManager2 via signature scanning (needed for center HTML display). */
bool InitGameEventManager();
/** Display HTML content in the player's center HUD via a game event. */
void SendCenterHtml(int slot, const std::string& html);
/** Send a chat message to a specific player via the SayText2 network message. */
void SendChatMessage(int slot, const std::string& message);
/** Clear any center HUD HTML for the given player. */
void ClearCenterHtml(int slot);

}  // namespace AdminSystem::Sdk
