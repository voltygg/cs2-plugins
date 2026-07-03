#pragma once

#include "../Actions/ActionContext.hpp"
#include "EffectId.hpp"

#include <functional>
#include <string>
#include <vector>

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;

/** Lifetime policy, declared as data on the descriptor (not baked into the body). */
enum class EffectScope
{
    Persistent, /**< Lives until toggled off, death, disconnect, or unload. */
    Round       /**< Also auto-cancels on round end/prestart. */
};

/**
 * @brief What an effect's setup body hands back: the two closures the kit's @ref EffectManager
 * drives. `OnTick` runs every `TickIntervalMs` (null for state-only effects); `OnStop` undoes
 * whatever was applied and runs exactly once when the effect ends for any reason.
 */
struct EffectInstance
{
    std::function<void()> OnTick;
    std::function<void()> OnStop;
};

/** One selectable option for a @ref ParamEffect submenu (Model uses its fun-model list). */
struct EffectChoice
{
    std::string Label;
    int Param;
};

/**
 * @brief A toggle / one-shot / timed player effect expressed as data, mirroring @ref Actions::Action.
 *
 * `Setup` receives the resolved, permission/immunity-checked context, applies the effect, and
 * returns its @ref EffectInstance. Lifetime is declarative: `Scope`, `TickIntervalMs`, and
 * `DurationMs` are data the dispatcher forwards to @ref EffectManager. An empty `OffKey` marks a
 * one-shot effect (no toggle-off broadcast).
 */
struct Effect
{
    std::string Flag; /**< Permission flag string; see AdminSystem::Flag(Permission). */
    int Id;           /**< = static_cast<int>(EffectId::X); key into the per-slot EffectManager map. */
    std::string NameKey;
    std::string OnKey;
    std::string OffKey;
    EffectScope Scope = EffectScope::Persistent;
    int TickIntervalMs = 0;
    int DurationMs = 0;
    bool RequireAlive = false;
    std::function<EffectInstance(const ActionContext&)> Setup;
};

/** Like @ref Effect but `Setup` receives a menu-supplied int (Model's selected model index). */
struct ParamEffect
{
    std::string Flag;
    int Id;
    std::string NameKey;
    std::string OnKey;
    std::string OffKey;
    std::string ResetLabelKey;
    EffectScope Scope = EffectScope::Persistent;
    int TickIntervalMs = 0;
    int DurationMs = 0;
    bool RequireAlive = false;
    std::function<std::vector<EffectChoice>()> Choices;
    std::function<EffectInstance(const ActionContext&, int param)> Setup;
};

/** Apply if inactive, clear if active. Broadcasts OnKey/OffKey. The default menu-row verb. */
void Toggle(int adminSlot, int targetSlot, const Effect& effect);
/** (Re)apply unconditionally, broadcasting OnKey. */
void Apply(int adminSlot, int targetSlot, const Effect& effect);
/** Cancel if active, broadcasting OffKey (when set). */
void Clear(int adminSlot, int targetSlot, const Effect& effect);

/** Apply the parameterized effect at `param`, broadcasting OnKey. */
void Apply(int adminSlot, int targetSlot, int param, const ParamEffect& effect);
/** Cancel the parameterized effect if active, broadcasting OffKey. */
void Clear(int adminSlot, int targetSlot, const ParamEffect& effect);

}  // namespace AdminSystem::Admin::Effects
