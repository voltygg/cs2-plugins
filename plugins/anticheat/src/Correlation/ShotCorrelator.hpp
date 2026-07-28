#pragma once

// The single engine feed every aim module runs off: one usercmd listener, one per-frame world
// snapshot and the four shot events, converted into the cores' plain samples.
//
// Aimbot, AntiAim and SilentAim have no adapter of their own, so they are dispatched from here.
// Aimlock's per-slot lag estimate lives apart in Detectors/AimlockDetector.hpp.

#include "Core/Samples.hpp"

#include <CS2Kit/Api.hpp>
#include <array>
#include <cstdint>

class IGameEvent;

namespace Anticheat
{

class AntiCheatManager;

class ShotCorrelator
{
public:
    explicit ShotCorrelator(AntiCheatManager& manager) : _manager(manager) {}

    /** Install the usercmd, per-frame and game-event listeners. */
    void Initialize();

private:
    void OnCommand(int slot, const CS2Kit::UserCmdView& cmd);
    void OnFrame();
    void OnWeaponFire(const CS2Kit::Events::WeaponFire& fire);
    void OnBulletImpact(const CS2Kit::Events::BulletImpact& impact);
    void OnPlayerHurt(IGameEvent* event);
    void OnPlayerDeath(const CS2Kit::Events::PlayerDeath& death);

    /** World state for the frame, plus the userid table bullet impacts resolve their shooter with. */
    void CollectPositions(std::array<PositionSample, MaxSlots>& players);

    /** Score every shot old enough that all of its events have arrived. */
    void FinalizeSilentAim(int slot, int32_t serverTick, double nowSec);

    AntiCheatManager& _manager;
    std::array<int32_t, MaxSlots> _userIds{};
    bool _userIdsResolved = false;  // false when the engine interface never answered
};

}  // namespace Anticheat
