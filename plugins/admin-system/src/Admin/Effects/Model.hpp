#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace AdminSystem::Admin::Effects
{

/** One selectable fun model: a display name plus its `.vmdl` resource path. */
struct FunModel
{
    std::string Name;
    std::string Path;
};

/** The curated, hardcoded fun-model list shown in the Effects > Model submenu. */
const std::vector<FunModel>& FunModels();

/** Queue every fun model plus the team-default restore models for precache. Call once in OnLoad;
 *  the paths only reach clients from the NEXT map load (see PrecacheService). */
void PrecacheModels(VoltMod::Runtime& runtime);

}  // namespace AdminSystem::Admin::Effects
