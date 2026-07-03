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
void PrecacheModels();

/** Apply the fun model at `modelIndex` (into FunModels()) to the target. Cancels any prior model
 *  effect first, requires the target alive, and broadcasts on success. */
void ApplyModel(int adminSlot, int targetSlot, std::size_t modelIndex);

/** Clear an active model effect on the target, restoring their team-default model while alive. */
void ResetModel(int adminSlot, int targetSlot);

}  // namespace AdminSystem::Admin::Effects
