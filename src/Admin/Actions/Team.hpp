#pragma once

namespace AdminSystem::Admin::Actions
{

void DoChangeTeam(int adminSlot, int targetSlot, int team);

constexpr int TeamSpec = 1;
constexpr int TeamT = 2;
constexpr int TeamCt = 3;

}  // namespace AdminSystem::Admin::Actions
