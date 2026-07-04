#pragma once

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <functional>
#include <vector>

namespace AdminSystem::Admin::Effects
{

/**
 * @brief A registered effect, type-erased behind a self-rendering `Render` hook so the menu loop
 * can list Effect and ParamEffect descriptors uniformly without knowing their concrete type.
 */
struct EffectEntry
{
    std::function<void(CS2Kit::MenuBuilder&, int admin, int target)> Render;
};

/** The effects auto-listed in the Effects action menu, built once on first use. */
const std::vector<EffectEntry>& EffectRegistry();

}  // namespace AdminSystem::Admin::Effects
