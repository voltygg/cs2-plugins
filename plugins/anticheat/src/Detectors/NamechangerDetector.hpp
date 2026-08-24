#pragma once

// Drives NamechangerCore from the two lifecycle points the engine gives us: the name a player
// arrives with, and every replicated settings change afterwards.

#include <VoltMod/Api.hpp>
#include <string>

namespace Anticheat
{

class AntiCheatManager;

class NamechangerDetector
{
public:
    explicit NamechangerDetector(AntiCheatManager& manager) : _manager(manager) {}

    /** Full connect: the first point the controller's name is meaningful. */
    void OnFullyConnected(VoltMod::Players::Player* player);

    /** A replicated settings change; only an actually different name counts. */
    void OnSettingsChanged(VoltMod::Players::Player* player);

private:
    /** The visible name for @p player, preferring the controller over the connect-time copy. */
    static std::string CurrentName(VoltMod::Players::Player* player);

    AntiCheatManager& _manager;
};

}  // namespace Anticheat
