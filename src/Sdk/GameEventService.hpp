#pragma once

#include "../Core/Singleton.hpp"

#include <cstdint>
#include <functional>
#include <igameevents.h>
#include <set>
#include <string>
#include <unordered_map>

namespace AdminSystem::Sdk
{

/**
 * Wrapper for IGameEventManager2 providing event creation, firing, and listener registration.
 * Implements IGameEventListener2 to dispatch events to registered callbacks.
 */
class GameEventService : public Core::Singleton<GameEventService>, public IGameEventListener2
{
public:
    explicit GameEventService(Token) {}

    bool Initialize();

    // Fire events
    IGameEvent* CreateEvent(const char* name);
    bool FireEvent(IGameEvent* event, bool dontBroadcast = false);
    void FreeEvent(IGameEvent* event);

    // Listen for events
    using EventCallback = std::function<void(IGameEvent*)>;
    uint64_t Listen(const char* eventName, EventCallback callback);
    void RemoveListener(uint64_t id);

    // IGameEventListener2 interface
    void FireGameEvent(IGameEvent* event) override;

private:
    struct RegisteredListener
    {
        std::string EventName;
        EventCallback Callback;
    };

    std::unordered_map<uint64_t, RegisteredListener> _listeners;
    uint64_t _nextListenerId = 1;
    std::set<std::string> _registeredEvents;
};

}  // namespace AdminSystem::Sdk
