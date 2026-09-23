#pragma once

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Slots/PerSlot.hpp>
#include <cstdint>

namespace AdminSystem::Admin::Actions
{

/** Slap and delayed slay. Their timers belong to the slot and drop when it changes hands, so
 *  neither can reach the next player. */
class PawnTimers
{
public:
    explicit PawnTimers(VoltMod::Runtime& runtime) : _runtime(runtime), _timers(runtime.Slots) {}
    PawnTimers(const PawnTimers&) = delete;
    PawnTimers& operator=(const PawnTimers&) = delete;

    /** Punt the pawn up with some sideways jitter. Godmode for @p fallProtectMs keeps the landing
     *  from killing it, unless the pawn already had godmode. */
    void Slap(const VoltMod::Pawn& pawn, float upward = 800.0f, float horizontal = 100.0f, int fallProtectMs = 3000);

    /** Slay @p slot's pawn after @p delayMs, replacing a slay already pending there. */
    void SlayAfter(int slot, int64_t delayMs);

private:
    struct Timers
    {
        VoltMod::Subscription FallProtect;
        VoltMod::Subscription Slay;
    };

    VoltMod::Runtime& _runtime;
    VoltMod::PerSlot<Timers> _timers;
};

}  // namespace AdminSystem::Admin::Actions
