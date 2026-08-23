#pragma once

#include "Descriptors.hpp"

#include <array>

namespace AdminSystem::Admin::Effects
{

/**
 * @brief One row in the Effects action menu.
 *
 * Exactly one of Toggle/Param is set, and the table below is the menu's order - listed in one
 * place rather than assembled from per-TU static initializers, so what the menu shows is
 * readable without opening every descriptor file. Hide is deliberately absent: it is a
 * self-only Control row plus the !hide command, not an auto-listed effect.
 */
struct EffectEntry
{
    const Effect* Toggle = nullptr;
    const ParamEffect* Param = nullptr;
};

/** Every auto-listed effect, in the order the menu renders them. */
inline constexpr std::array MenuEffects{
    EffectEntry{.Toggle = &Ghost}, EffectEntry{.Toggle = &Disco}, EffectEntry{.Toggle = &Wallhack},
    EffectEntry{.Param = &Model},  EffectEntry{.Toggle = &Bhop},
};

}  // namespace AdminSystem::Admin::Effects
