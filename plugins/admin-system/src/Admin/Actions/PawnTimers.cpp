#include "Admin/Actions/PawnTimers.hpp"

#include <VoltMod/Runtime.hpp>
#include <random>

namespace AdminSystem::Admin::Actions
{

static float Jitter(float range)
{
    static std::mt19937 generator{std::random_device{}()};
    return std::uniform_real_distribution<float>(-range, range)(generator);
}

void PawnTimers::Slap(const VoltMod::Pawn& pawn, float upward, float horizontal, int fallProtectMs)
{
    pawn.Launch(Vector{Jitter(horizontal), Jitter(horizontal), upward});

    const int slot = pawn.Slot();
    if (fallProtectMs <= 0 || !VoltMod::IsValidSlot(slot) || pawn.Godmode())
    {
        return;
    }

    pawn.SetGodmode(true);
    // Resolved again when it fires: the pawn is only valid this frame.
    _timers[slot].FallProtect =
        _runtime.Scheduler.Delay(fallProtectMs, [this, slot] { _runtime.Entities.Pawn(slot).SetGodmode(false); });
}

void PawnTimers::SlayAfter(int slot, int64_t delayMs)
{
    if (!VoltMod::IsValidSlot(slot))
    {
        return;
    }

    _timers[slot].Slay = _runtime.Scheduler.Delay(delayMs, [this, slot] {
        if (const VoltMod::Pawn pawn = _runtime.Entities.Pawn(slot); pawn.IsAlive())
        {
            (void)pawn.Slay();
        }
    });
}

}  // namespace AdminSystem::Admin::Actions
