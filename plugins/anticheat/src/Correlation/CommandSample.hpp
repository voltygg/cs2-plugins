#pragma once

// Turns a decoded usercmd plus the pawn it drives into the plain sample the cores read.

#include "Core/Samples.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Hooks/Api.hpp>

namespace Anticheat
{

/** The usercmd's own fields, with nothing read from the pawn. */
CmdSample BuildSample(const VoltMod::PlayerInput& cmd);

/** Eye, air state, scope and the predicted recoil punch, read before the engine simulates @p cmd. */
void StampPawnState(CmdSample& sample, const VoltMod::Pawn& pawn);

bool IsAirborne(const VoltMod::Pawn& pawn);

}  // namespace Anticheat
