#pragma once

#include "../Core/Singleton.hpp"

#include <string>

class IGameEventListener2;
class INetworkMessageInternal;
class CPlayerSlot;

namespace AdminSystem::Sdk
{

/** Network message ID for the SayText2 user message in CS2. */
constexpr int UmSayText2 = 118;

/**
 * Message system for sending chat and center HTML messages to players.
 *
 * Wraps two engine subsystems:
 * - **IGameEventSystem** + **INetworkMessages**: for sending protobuf-based
 *   network messages (chat via SayText2).
 * - **IGameEventManager2**: for firing game events that trigger the center
 *   HUD HTML display (show_survival_respawn_status).
 *
 * @note IGameEventManager2 must be resolved separately via InitGameEventManager()
 *       after signature scanning is available, because it is not exposed through
 *       standard Metamod interface queries.
 *
 * @see EntitySystem For player slot validation.
 * @see GameData For signature resolution used by InitGameEventManager().
 */
class MessageSystem : public Core::Singleton<MessageSystem>
{
public:
    explicit MessageSystem(Token) {}

    /**
     * Initialize the network message system.
     *
     * Resolves the SayText2 network message internal pointer via
     * INetworkMessages. Requires GameInterfaces::GameEventSystem and
     * GameInterfaces::NetworkMessages to be set.
     *
     * @return True on success, false if required interfaces are missing.
     */
    bool Initialize();

    /**
     * Resolve IGameEventManager2 via signature scanning.
     *
     * This is a separate step from Initialize() because it depends on
     * GameData being loaded and SigScanner being available.
     *
     * @return True if the game event manager was found, false otherwise.
     * @note Without this, SendCenterHtml() and ClearCenterHtml() will not work.
     */
    bool InitGameEventManager();

    /**
     * Display HTML content in the player's center HUD.
     *
     * Uses the `show_survival_respawn_status` game event, which renders
     * arbitrary HTML in the center of the screen.
     *
     * @param slot Player slot index (0-63).
     * @param html HTML string to display.
     */
    void SendCenterHtml(int slot, const std::string& html);

    /**
     * Send a chat message to a specific player.
     *
     * Uses the SayText2 protobuf network message sent via IGameEventSystem.
     *
     * @param slot    Player slot index (0-63).
     * @param message Chat message text (supports color codes).
     */
    void SendChatMessage(int slot, const std::string& message);

    /**
     * Clear the center HUD HTML for a player.
     *
     * Sends an empty HTML string to remove any currently displayed content.
     *
     * @param slot Player slot index (0-63).
     */
    void ClearCenterHtml(int slot);

private:
    /** Function pointer type for resolving legacy game event listeners by slot. */
    using GetLegacyGameEventListenerFn = IGameEventListener2* (*)(CPlayerSlot slot);

    GetLegacyGameEventListenerFn _getLegacyListener = nullptr;  ///< Resolved via signature scan.
    INetworkMessageInternal* _sayText2Internal = nullptr;       ///< SayText2 message interface.
};

}  // namespace AdminSystem::Sdk
