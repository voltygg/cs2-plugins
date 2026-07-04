#pragma once

#include "EffectDescriptor.hpp"

#include <CS2Kit/Core/Registry.hpp>

namespace AdminSystem::Admin::Effects
{

/**
 * @brief One auto-listed row in the Effects action menu. Data-only so each descriptor .cpp
 * self-registers at its definition site:
 *
 *   static const bool _registered = CS2Kit::Registry<EffectEntry>::Add({.Order = 10, .Toggle = &Ghost});
 *
 * Exactly one of Toggle/Param is set. Registration order across TUs is unspecified, so the
 * menu sorts by Order. Hide is deliberately not registered - it is a self-only Control row
 * plus the !hide command, not an auto-listed effect.
 */
struct EffectEntry
{
    int Order = 0;
    const Effect* Toggle = nullptr;
    const ParamEffect* Param = nullptr;
};

}  // namespace AdminSystem::Admin::Effects
