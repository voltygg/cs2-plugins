#pragma once

#include "../Core/Singleton.hpp"

#include <igameevents.h>

#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <unordered_map>

namespace AdminSystem::Sdk
{

/**
 * Wrapper for IGameEventManager2 providing event creation, firing, and listener registration.
 *
 * Implements IGameEventListener2 to receive engine event dispatches and route
 * them to registered plugin callbacks by event name.
 *
 * @note IGameEventManager2 is resolved via signature scanning in MessageSystem::InitGameEventManager().
 *       If that step fails, this service will not initialize.
 *
 * @par Usage
 * @code
 * // Listen for round start:
 * auto id = GameEventService::Instance().Listen("round_start", [](IGameEvent* e) {
 *     Log::Info("Round started!");
 * });
 *
 * // Create and fire a custom event:
 * if (auto* evt = GameEventService::Instance().CreateEvent("my_event"))
 * {
 *     evt->SetBool("success", true);
 *     GameEventService::Instance().FireEvent(evt);
 * }
 *
 * // Unregister:
 * GameEventService::Instance().RemoveListener(id);
 * @endcode
 */
class GameEventService : public Core::Singleton<GameEventService>, public IGameEventListener2
{
public:
    explicit GameEventService(Token) {}

    /**
     * Initialize the service.
     * @return True if IGameEventManager2 is available, false otherwise.
     */
    bool Initialize();

    /// @name Event Creation & Firing
    /// @{

    /**
     * Create a new game event by name.
     * @param name Event name (e.g. "round_start").
     * @return Pointer to the new event, or nullptr if creation failed.
     * @note The caller must either fire or free the returned event.
     */
    IGameEvent* CreateEvent(const char* name);

    /**
     * Fire a previously created game event.
     * @param event         Event to fire (ownership is transferred to the engine).
     * @param dontBroadcast If true, the event is not sent to clients.
     * @return True on success.
     */
    bool FireEvent(IGameEvent* event, bool dontBroadcast = false);

    /**
     * Free a game event without firing it.
     * @param event Event to free.
     */
    void FreeEvent(IGameEvent* event);

    /// @}

    /// @name Listener Registration
    /// @{

    /** Callback signature for event listeners. */
    using EventCallback = std::function<void(IGameEvent*)>;

    /**
     * Register a callback for a specific game event.
     *
     * The engine listener is registered lazily on the first call for each
     * distinct event name. Multiple callbacks can be registered for the same event.
     *
     * @param eventName Event name to listen for (e.g. "player_death").
     * @param callback  Function to invoke when the event fires.
     * @return Listener ID that can be passed to RemoveListener(), or 0 on failure.
     */
    uint64_t Listen(const char* eventName, EventCallback callback);

    /**
     * Remove a previously registered listener.
     * @param id Listener ID returned by Listen().
     */
    void RemoveListener(uint64_t id);

    /// @}

    /**
     * IGameEventListener2 callback invoked by the engine.
     *
     * Dispatches the event to all registered callbacks whose event name matches.
     *
     * @param event The game event that just occurred.
     */
    void FireGameEvent(IGameEvent* event) override;

private:
    /** Internal storage for a registered listener. */
    struct RegisteredListener
    {
        std::string EventName;   ///< Event name this listener is subscribed to.
        EventCallback Callback;  ///< Function to invoke.
    };

    std::unordered_map<uint64_t, RegisteredListener> _listeners;  ///< All registered listeners.
    uint64_t _nextListenerId = 1;                                 ///< Monotonically increasing ID.
    std::set<std::string> _registeredEvents;                      ///< Events for which AddListener has been called.
};

}  // namespace AdminSystem::Sdk
