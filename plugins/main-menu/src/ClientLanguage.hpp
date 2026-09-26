#pragma once

#include <VoltMod/Core/Signals/Subscription.hpp>
#include <VoltMod/Players/Player.hpp>
#include <VoltMod/Runtime.hpp>

namespace MainMenu
{

/**
 * Gives a player who has not picked a language the one their Steam client runs in (`cl_language`).
 * The player language lives in the host, so this one query sets it for every plugin; a pick in
 * settings still overrides it.
 */
class ClientLanguage
{
public:
    /** Subscribes to fully connected players. */
    explicit ClientLanguage(VoltMod::Runtime& runtime);

private:
    void OnFullyConnected(VoltMod::Player& player);

    VoltMod::Runtime& _rt;
    /** Declared last so the handler stops before the state it captures. */
    VoltMod::Subscription _connected;
};

}  // namespace MainMenu
