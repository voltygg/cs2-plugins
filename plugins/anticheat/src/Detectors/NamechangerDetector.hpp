#pragma once

// Drives NamechangerCore from the two lifecycle points the engine gives us: the name a player
// arrives with, and every replicated settings change afterwards.

#include "../AnticheatTypes.hpp"

#include <VoltMod/Api.hpp>
#include <string>

namespace Anticheat
{

class NamechangerDetector
{
public:
    NamechangerDetector(AntiCheatManager& manager, VoltMod::Runtime& runtime) : _manager(manager), _rt(runtime) {}

    /** Full connect: the first point the controller's name is meaningful. */
    void OnFullyConnected(VoltMod::Player* player);

    /** A replicated settings change; only an actually different name counts. */
    void OnSettingsChanged(VoltMod::Player* player);

private:
    /** The visible name for @p player, preferring the controller over the connect-time copy. */
    std::string CurrentName(VoltMod::Player* player) const;

    AntiCheatManager& _manager;
    VoltMod::Runtime& _rt;
};

}  // namespace Anticheat
