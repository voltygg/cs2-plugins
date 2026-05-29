#pragma once

#include "../Actions/ActionContext.hpp"
#include "EffectId.hpp"
#include "EffectManager.hpp"

#include <functional>
#include <string>

namespace AdminSystem::Admin::Effects
{

/**
 * @brief A toggleable effect expressed as data, mirroring @ref Actions::Action.
 *
 * `Enable` receives the resolved, permission/immunity-checked context, applies the effect's
 * state, and returns the @ref EffectSetup (timer + cancel closure) that @ref EffectManager
 * registers. Re-invoking on an already-active effect cancels it. The dispatcher owns the
 * `Resolve -> Toggle -> Broadcast(on ? OnKey : OffKey)` shape so every effect is just data.
 */
struct EffectToggle
{
    Permission Flag;
    EffectId Id;
    std::string OnKey;
    std::string OffKey;
    std::function<EffectSetup(const Actions::ActionContext&)> Enable;
};

void Run(int adminSlot, int targetSlot, const EffectToggle& effect);

}  // namespace AdminSystem::Admin::Effects
