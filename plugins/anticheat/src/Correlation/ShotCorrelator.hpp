#pragma once

// The single engine feed every aim module runs off: one usercmd listener, one per-frame world
// snapshot and the four shot events, converted into the cores' plain samples.
//
// Aimbot, AntiAim and SilentAim have no adapter of their own, so they are dispatched from here.
// Aimlock's per-slot lag estimate lives apart in Detectors/AimlockDetector.hpp.

#include "../AnticheatTypes.hpp"
#include "Core/Samples.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Engine/EngineTypes.hpp>
#include <array>
#include <cstdint>
#include <vector>

namespace Anticheat
{

class ShotCorrelator
{
public:
    ShotCorrelator(AntiCheatManager& manager, VoltMod::Runtime& runtime) : _manager(manager), _rt(runtime) {}

    /** Install the usercmd, per-frame and game-event listeners. */
    void Initialize();

private:
    void OnCommand(int slot, const VoltMod::UserCmdView& cmd);
    void OnFrame();
    void OnWeaponFire(const VoltMod::WeaponFire& fire);
    void OnBulletImpact(const VoltMod::BulletImpact& impact);
    void OnPlayerHurt(const VoltMod::PlayerHurt& hurt);
    void OnPlayerDeath(const VoltMod::PlayerDeath& death);

    /** World state for the frame, plus the userid table bullet impacts resolve their shooter with. */
    void CollectPositions(std::array<PositionSample, MaxSlots>& players);

    /** Score every shot old enough that all of its events have arrived. */
    void FinalizeSilentAim(int slot, int32_t serverTick, double nowSec);

    /** True while @p slot is inside the post-teleport window, where origin and view angles have
     *  jumped and anything measuring motion across ticks reads as impossible. */
    bool JustTeleported(int slot) const;

    AntiCheatManager& _manager;
    VoltMod::Runtime& _rt;
    std::array<int32_t, MaxSlots> _userIds{};
    /** Server time of each slot's last teleport, 0 for none. Fed by the framework's Teleported
     *  event, which is also what arms the per-pawn hook; bound to Slots.Changed so a stamp can
     *  never outlive its player. */
    VoltMod::PerSlot<float> _lastTeleport;
    bool _userIdsResolved = false;  // false when the engine interface never answered

    /** Registrations, released together. Declared last: reverse member destruction stops the
     *  handlers before the state they write to goes away. */
    std::vector<VoltMod::Subscription> _subscriptions;
};

}  // namespace Anticheat
