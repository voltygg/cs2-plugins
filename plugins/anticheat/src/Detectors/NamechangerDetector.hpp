#pragma once

// Drives NamechangerCore from what the scoreboard shows: the identity a player arrives with,
// every replicated settings change, and a periodic read of the controller for the changes that
// reach it without one (a clan tag pushed by a cheat, for instance).

#include "AnticheatTypes.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Subscription.hpp>

namespace Anticheat
{

class NamechangerDetector
{
public:
    NamechangerDetector(AntiCheatManager& manager, VoltMod::Runtime& runtime) : _manager(manager), _rt(runtime) {}

    /** Install the periodic controller read. */
    void Initialize();

    /** Full connect: the first point the controller's name is meaningful. */
    void OnFullyConnected(VoltMod::Player& player);

    /** A replicated settings change; only an actually different name or tag counts. */
    void OnSettingsChanged(VoltMod::Player& player);

private:
    /** Both gates the identity reads answer to: detections at all, and this module. */
    bool Enabled() const;
    void CheckIdentity(int slot, double nowSec);
    void OnPoll();

    AntiCheatManager& _manager;
    VoltMod::Runtime& _rt;
    VoltMod::Subscription _pollTimer;
};

}  // namespace Anticheat
