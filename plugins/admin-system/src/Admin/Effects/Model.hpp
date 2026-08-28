#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace AdminSystem::Admin::Effects
{

/** One selectable fun model: a display name plus its `.vmdl` resource path. */
struct FunModel
{
    std::string Name;
    std::string Path;
};

/** The chicken model, shared with Fun Mode's chicken-bots round toggle. Precached by
 *  @ref PrecacheModels, so both users get it from the same queue. */
inline constexpr std::string_view ChickenModelPath = "models/chicken/chicken.vmdl";

/** The curated, hardcoded fun-model list shown in the Effects > Model submenu. */
const std::vector<FunModel>& FunModels();

/** Queue every fun model plus the team-default restore models for precache. Call once in OnLoad;
 *  the paths only reach clients from the NEXT map load (see Precache). */
void PrecacheModels(VoltMod::Runtime& runtime);

}  // namespace AdminSystem::Admin::Effects
