#pragma once

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Slots/PerSlot.hpp>
#include <chrono>
#include <cstdint>

namespace AdminSystem::Admin::Actions
{

/** Slap and delayed slay. Their state belongs to the slot and drops when it changes hands, so
 *  neither can reach the next player. */
class PawnTimers
{
public:
    explicit PawnTimers(VoltMod::Runtime& runtime);
    PawnTimers(const PawnTimers&) = delete;
    PawnTimers& operator=(const PawnTimers&) = delete;

    /** Punt the pawn up with some sideways jitter. Fall damage to that pawn is blocked for
     *  @p fallProtectMs so the landing cannot kill it. */
    void Slap(const VoltMod::Pawn& pawn, float upward = 800.0f, float horizontal = 100.0f, int fallProtectMs = 3000);

    /** Slay @p slot's pawn after @p delayMs, replacing a slay already pending there. */
    void SlayAfter(int slot, int64_t delayMs);

private:
    struct Timers
    {
        VoltMod::EntityRef Slapped;
        std::chrono::steady_clock::time_point FallSafeUntil;
        VoltMod::Subscription Slay;
    };

    void OnDamage(VoltMod::DamageHit& hit);

    VoltMod::Runtime& _runtime;
    VoltMod::PerSlot<Timers> _timers;
    VoltMod::Subscription _damage;
};

}  // namespace AdminSystem::Admin::Actions
