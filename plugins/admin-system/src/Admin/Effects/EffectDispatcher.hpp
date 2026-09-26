#pragma once

#include "Admin/Actions/ActionDispatcher.hpp"
#include "Admin/Effects/EffectDescriptor.hpp"
#include "Admin/Effects/EffectManager.hpp"

#include <VoltMod/Players/PlayerRef.hpp>

namespace AdminSystem::Admin::Effects
{

/** Runs data-defined effects (@ref EffectDescriptor) against a player, the effect-side counterpart
 *  of @ref Actions::ActionDispatcher. */
class EffectDispatcher
{
public:
    /** Resolves and announces through @p actions; @p effects holds the state. Both must outlive
     *  the dispatcher. */
    EffectDispatcher(Actions::ActionDispatcher& actions, EffectManager& effects) : _actions(actions), _effects(effects)
    {}

    EffectDispatcher(const EffectDispatcher&) = delete;
    EffectDispatcher& operator=(const EffectDispatcher&) = delete;

    /** Apply if inactive, clear if active. Broadcasts OnKey/OffKey. The default menu-row verb.
     *  @p param is forwarded to `Setup` (0 for a plain toggle). */
    void Toggle(VoltMod::PlayerRef admin, VoltMod::PlayerRef target, const EffectDescriptor& effect,
                int param = 0) const;
    /** (Re)apply unconditionally, broadcasting OnKey. */
    void Apply(VoltMod::PlayerRef admin, VoltMod::PlayerRef target, const EffectDescriptor& effect,
               int param = 0) const;
    /** Cancel if active, broadcasting OffKey (when set). */
    void Clear(VoltMod::PlayerRef admin, VoltMod::PlayerRef target, const EffectDescriptor& effect) const;

private:
    Actions::ActionDispatcher& _actions;
    EffectManager& _effects;
};

}  // namespace AdminSystem::Admin::Effects
