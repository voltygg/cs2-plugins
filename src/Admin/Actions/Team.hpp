#pragma once

#include "ActionContext.hpp"

namespace AdminSystem::Admin::Actions
{

constexpr int TeamSpec = 1;
constexpr int TeamT = 2;
constexpr int TeamCt = 3;

/** Param is the destination team (TeamSpec/TeamT/TeamCt); out-of-range values are ignored. */
extern const ParamAction ChangeTeam;

}  // namespace AdminSystem::Admin::Actions
