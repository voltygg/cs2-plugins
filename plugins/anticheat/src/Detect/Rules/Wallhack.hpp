#pragma once

#include "Detect/Finding.hpp"
#include "Detect/Samples.hpp"
#include "Detect/ShotHistory.hpp"
#include "Detect/Suspicion.hpp"
#include "Detect/ViewLag.hpp"

#include <array>

namespace Anticheat::Rules
{

/** What the engine adapter can add about a finalized shot that the frames do not hold. */
struct WallhackShotContext
{
    /** A teammate of the shooter has line of sight to the victim, or had it within the last seconds. */
    bool TeamSawVictim = false;
};

class Wallhack
{
public:
    /** The settings toggle and catalog entry this rule reports under. */
    static constexpr DetectionKind Kind = DetectionKind::Wallhack;

    Wallhack(const ShotHistory& shots, Suspicion& suspicion) : _shots(shots), _suspicion(suspicion) {}

    void Reset();
    void ClearSlot(int slot);

    /** The command the server simulates for @p serverTick. */
    void OnSimulated(int slot, int32_t serverTick, const AimAngles& angles, const Vec3& eyePos);

    /** Advance or close the through-wall tracking episode against the frame for @p serverTick. */
    void OnFrame(int slot, int32_t serverTick, bool aliveHuman, const ViewLag& lag, double nowSec);

    /** Every ballistic fire event, matched or not: gunfire gives a hidden player's position away. */
    void OnWeaponFire(int slot, int32_t fireTick);

    /** A finalized shot; judged only when it hurt someone. */
    void OnShot(int slot, const ShotView& shot, const WallhackShotContext& context, double nowSec);

    bool IsTracking(int slot) const;

private:
    struct AimSample
    {
        int32_t ServerTick = -1;
        AimAngles Angles;
        Vec3 EyePos;
        bool Valid = false;
    };

    /** One run of aim resting on a hidden enemy. Signed yaw travel tells following from waiting. */
    struct Track
    {
        int Target = -1;
        int32_t StartTick = -1;
        int32_t LastTick = -1;
        int Samples = 0;
        int OnSamples = 0;
        int OffRun = 0;
        AimAngles LastAim;
        AimAngles LastBearing;
        float AimYawTravel = 0.0f;
        float BearingYawTravel = 0.0f;
        bool Qualified = false;
    };

    struct SlotData
    {
        AimSample Pending;
        Track Current;
        int32_t CooldownUntilTick = -1;
        /** The hidden enemy the aim followed until it stepped into view, and when. */
        int PeekTarget = -1;
        int32_t PeekTick = -1;
        int32_t LastFireTick = -1;
    };

    /** Every report this rule makes goes through here, so the reason always reads the same way.
     *  True when the added points crossed a band. */
    bool Report(int slot, int points, std::string reason, double nowSec);
    void CloseTrack(SlotData& data, int32_t serverTick, bool becameVisible);
    /** How fast @p slot moved over the ticks before @p serverTick, in units per second. */
    float Speed(int slot, int32_t serverTick) const;

    const ShotHistory& _shots;
    Suspicion& _suspicion;
    std::array<SlotData, MaxSlots> _slots{};
};

}  // namespace Anticheat::Rules
