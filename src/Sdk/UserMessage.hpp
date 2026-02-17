#pragma once

#include "../Core/Singleton.hpp"

#include <string>

class IGameEventListener2;
class INetworkMessageInternal;
class CPlayerSlot;

namespace AdminSystem::Sdk
{

/** Network message ID for the SayText2 user message. */
constexpr int UmSayText2 = 118;

/**
 * Message system for sending chat and center HTML messages to players.
 * Wraps IGameEventSystem (for protobuf messages) and IGameEventManager2 (for center HTML events).
 */
class MessageSystem : public Core::Singleton<MessageSystem>
{
public:
    explicit MessageSystem(Token) {}

    /** Initialize the network message system (requires GameEventSystem and NetworkMessages). */
    bool Initialize();

    /** Resolve IGameEventManager2 via signature scanning (needed for center HTML display). */
    bool InitGameEventManager();

    /** Display HTML content in the player's center HUD via a game event. */
    void SendCenterHtml(int slot, const std::string& html);

    /** Send a chat message to a specific player via the SayText2 network message. */
    void SendChatMessage(int slot, const std::string& message);

    /** Clear any center HUD HTML for the given player. */
    void ClearCenterHtml(int slot);

private:
    using GetLegacyGameEventListenerFn = IGameEventListener2* (*)(CPlayerSlot slot);
    GetLegacyGameEventListenerFn _getLegacyListener = nullptr;
    INetworkMessageInternal* _sayText2Internal = nullptr;
};

}  // namespace AdminSystem::Sdk
