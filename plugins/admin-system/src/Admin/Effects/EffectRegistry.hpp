#pragma once

#include "Descriptors.hpp"

#include <array>

namespace AdminSystem::Admin::Effects
{

/**
 * @brief Effects-menu row with exactly one descriptor set.
 *
 * MenuEffects defines display order explicitly. Hide is
 * a self-only Control row
 * and `!hide` command, so it is not listed here.
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
