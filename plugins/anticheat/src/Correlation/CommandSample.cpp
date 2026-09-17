#include "Correlation/CommandSample.hpp"

#include "Core/Geometry.hpp"

#include <algorithm>
#include <cmath>
#include <mathlib/vector.h>

namespace Anticheat
{

bool IsAirborne(const VoltMod::Pawn& pawn)
{
    const bool grounded = pawn.OnGroundLastTick() || ((pawn.Flags() & VoltMod::FL_ONGROUND) &&
                                                      pawn.GroundEntity() != VoltMod::InvalidEntityHandle);
    return !grounded && pawn.Move() == VoltMod::MoveType::Walk &&
           pawn.ActualMoveTypeRaw() == static_cast<VoltMod::Schema::MoveType_t>(VoltMod::MoveType::Walk);
}

CmdSample BuildSample(const VoltMod::PlayerInput& cmd)
{
    CmdSample sample;
    sample.CmdNum = cmd.CommandNumber;
    sample.ClientTick = cmd.ClientTick;
    sample.ViewPitch = cmd.ViewPitch;
    sample.ViewYaw = cmd.ViewYaw;
    sample.ViewRoll = cmd.ViewRoll;
    sample.MouseDx = cmd.MouseDx;
    sample.MouseDy = cmd.MouseDy;
    sample.Buttons = cmd.ButtonsHeld;
    // A command that carried no viewangles leaves the fields at a perfectly ordinary-looking
    // (0,0,0), so the angles have to be untrusted rather than merely finite.
    sample.BaseAnglesFinite =
        cmd.HasViewAngles && Geometry::IsFinite(sample.BaseAngles()) && std::isfinite(sample.ViewRoll);

    for (int i = 0; i < cmd.SubtickMoveCount; ++i)
    {
        const VoltMod::SubtickMove& move = cmd.SubtickMoves[i];
        sample.SubtickPitchDelta += move.PitchDelta;
        sample.SubtickYawDelta += move.YawDelta;
        sample.SubtickAnglesFinite =
            sample.SubtickAnglesFinite && std::isfinite(move.PitchDelta) && std::isfinite(move.YawDelta);
    }

    const int attackIndex = cmd.Attack1StartHistoryIndex;
    sample.AttackStarted = attackIndex >= 0;
    // Only an index the client never sent is a fabrication. One the transport cap dropped is merely
    // absent, and must never be clamped back into range - that reads another shot's angles.
    sample.AttackIndexInvalid = attackIndex < -1 || attackIndex >= cmd.InputHistoryTotalCount;
    if (auto attack = cmd.SampleAt(attackIndex); attack && attack->HasViewAngles)
        sample.AttackAngles = AimAngles{attack->ViewPitch, attack->ViewYaw};

    for (int i = 0; i < cmd.InputHistorySampleCount; ++i)
    {
        const VoltMod::InputHistorySample& entry = cmd.InputHistorySamples[i];
        if (!entry.HasViewAngles)
            continue;
        sample.HasHistoryAngles = true;
        if (!std::isfinite(entry.ViewPitch) || !std::isfinite(entry.ViewYaw))
        {
            sample.HistoryAnglesFinite = false;
            continue;
        }
        sample.MaxHistoryYawDelta =
            std::max(sample.MaxHistoryYawDelta, std::abs(Geometry::YawDelta(sample.ViewYaw, entry.ViewYaw)));
    }
    return sample;
}

void StampPawnState(CmdSample& sample, const VoltMod::Pawn& pawn)
{
    const Vector eye = pawn.EyePosition();
    sample.EyePos = {eye.x, eye.y, eye.z};
    sample.Airborne = IsAirborne(pawn);
    sample.Scoped = pawn.Scoped();

    // The punch the client predicted for this command is the one the last shot left behind.
    const VoltMod::Schema::CCSPlayer_AimPunchServices punch = pawn.AimPunchServices();
    if (punch)
    {
        const QAngle base = punch.BaseAngle();
        sample.Punch = {base.x, base.y};
        sample.HasPunch = Geometry::IsFinite(sample.Punch);
    }
}

}  // namespace Anticheat
