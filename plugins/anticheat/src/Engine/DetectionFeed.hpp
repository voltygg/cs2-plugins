#pragma once

#include "Detect/Samples.hpp"
#include "Engine/SightLines.hpp"
#include "Engine/Detectors.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Subscriptions.hpp>
#include <VoltMod/Engine/EngineTypes.hpp>
#include <VoltMod/Hooks/Api.hpp>
#include <array>
#include <cstdint>

namespace Anticheat
{

class DetectionFeed
{
public:
    DetectionFeed(Detectors& detectors, VoltMod::Runtime& runtime) : _detectors(detectors), _rt(runtime) {}

    /** Install the usercmd, per-frame and game-event listeners. */
    void Initialize();

    /** Why sight lines cannot be traced this map, when they cannot. */
    VoltMod::Status SightAvailable() const { return _sight.Available(); }

private:
    void OnCommand(int slot, const VoltMod::PlayerInput& cmd);
    void OnFrame();
    void OnWeaponFire(const VoltMod::WeaponFire& fire);
    void OnBulletImpact(const VoltMod::BulletImpact& impact);
    void OnPlayerHurt(const VoltMod::PlayerHurt& hurt);
    void OnPlayerDeath(const VoltMod::PlayerDeath& death);

    /** Collect frame state, each pawn's aim, and the userid table used to resolve impacts. */
    void CollectPositions(std::array<PositionSample, MaxSlots>& players, std::array<AimAngles, MaxSlots>& aims,
                          std::array<bool, MaxSlots>& viewers);

    /** Judge every shot old enough that all of its events have arrived. */
    void FinalizeShots(int slot, int32_t serverTick, double nowSec);

    /** A teammate of @p shooter can see @p victim now, or could within the last frames. */
    bool TeamSawVictim(int shooter, int victim, int32_t fireTick) const;

    /** True while post-teleport motion samples are invalid for @p slot. */
    bool RecentlyTeleported(int slot) const;

    Detectors& _detectors;
    VoltMod::Runtime& _rt;
    SightLines _sight{_rt};
    std::array<int32_t, MaxSlots> _userIds{};
    /** Server time of each slot's last teleport, or 0 when none. */
    VoltMod::PerSlot<float> _lastTeleport;
    bool _userIdsResolved = false;  // false when the engine interface never answered

    /** Registrations released after the state they capture. */
    VoltMod::Subscriptions _subscriptions;
};

}  // namespace Anticheat
