#pragma once

// Drives NamechangerCore from the two lifecycle points the engine gives us: the name a player
// arrives with, and every replicated settings change afterwards.

#include "../AnticheatTypes.hpp"

#include <VoltMod/Api.hpp>

namespace Anticheat
{

class NamechangerDetector
{
public:
    explicit NamechangerDetector(AntiCheatManager& manager) : _manager(manager) {}

    /** Full connect: the first point the controller's name is meaningful. */
    void OnFullyConnected(VoltMod::Player& player);

    /** A replicated settings change; only an actually different name counts. */
    void OnSettingsChanged(VoltMod::Player& player);

private:
    AntiCheatManager& _manager;
};

}  // namespace Anticheat
