#pragma once

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Slots/PerSlot.hpp>
#include <VoltMod/Core/Signals/Subscriptions.hpp>

namespace Anticheat
{

/**
 * Logs raw usercmds for one slot on demand (`anticheat_dumpcmd`). No part in detection, so it
 * owns its own movement hook and console command rather than sharing the detection wiring.
 */
class CommandDump
{
public:
    explicit CommandDump(VoltMod::Runtime& runtime) : _rt(runtime) {}

    /** Arm the movement hook and register `anticheat_dumpcmd`. */
    void Initialize();

private:
    void OnCommand(int slot, const VoltMod::PlayerInput& cmd);

    VoltMod::Runtime& _rt;
    VoltMod::PerSlot<int> _remaining;  // ticks still to dump, per slot
    VoltMod::Subscriptions _subs;
};

}  // namespace Anticheat
