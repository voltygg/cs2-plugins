#pragma once

// Converts usercmd, frame, and shot-event feeds into samples for the aim modules.

#include "../AnticheatTypes.hpp"
#include "Core/Samples.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Subscriptions.hpp>
#include <VoltMod/Engine/EngineTypes.hpp>
#include <VoltMod/Hooks/Api.hpp>
#include <array>
#include <cstdint>

namespace Anticheat
{

class ShotCorrelator
{
public:
    ShotCorrelator(AntiCheatManager& manager, VoltMod::Runtime& runtime) : _manager(manager), _rt(runtime) {}

    /** Install the usercmd, per-frame and game-event listeners. */
    void Initialize();

private:
    void OnCommand(int slot, const VoltMod::PlayerInput& cmd);
    void OnFrame();
    void OnWeaponFire(const VoltMod::WeaponFire& fire);
    void OnBulletImpact(const VoltMod::BulletImpact& impact);
    void OnPlayerHurt(const VoltMod::PlayerHurt& hurt);
    void OnPlayerDeath(const VoltMod::PlayerDeath& death);

    /** Collect frame state and the userid table used to resolve impacts. */
    void CollectPositions(std::array<PositionSample, MaxSlots>& players);

    /** Score every shot old enough that all of its events have arrived. */
    void FinalizeSilentAim(int slot, int32_t serverTick, double nowSec);

    /** True while post-teleport motion samples are invalid for @p slot. */
    bool JustTeleported(int slot) const;

    AntiCheatManager& _manager;
    VoltMod::Runtime& _rt;
    std::array<int32_t, MaxSlots> _userIds{};
    /** Server time of each slot's last teleport, or 0 when none. */
    VoltMod::PerSlot<float> _lastTeleport;
    bool _userIdsResolved = false;  // false when the engine interface never answered

    /** Registrations released after the state they capture. */
    VoltMod::Subscriptions _subscriptions;
};

}  // namespace Anticheat
