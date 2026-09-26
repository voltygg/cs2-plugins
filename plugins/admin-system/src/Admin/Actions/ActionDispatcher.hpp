#pragma once

#include "Admin/Actions/ActionContext.hpp"

#include <VoltMod/Core/Result.hpp>
#include <VoltMod/Players/PlayerRef.hpp>
#include <VoltMod/Players/Policy.hpp>
#include <functional>
#include <string_view>

namespace AdminSystem::Admin::Actions
{

/**
 * @brief Runs data-defined actions through `Policy::Authorize`, which supplies the permission and
 * targetability checks, and announces each one through @ref OnBroadcast.
 */
class ActionDispatcher
{
public:
    /** @p policy must outlive the dispatcher. */
    explicit ActionDispatcher(VoltMod::Policy& policy) : _policy(policy) {}

    /** Announces a performed action; unset stays silent. */
    std::function<void(const VoltMod::Authorized& who, std::string_view translationKey)> OnBroadcast;

    /**
     * Authorize a caller+target pair and build the context for it.
     *
     * Takes refs, not slots, and hands them to `Policy::Authorize` unchanged: a stored row or
     * callback that outlived its player is refused rather than retargeted at whoever occupies the
     * slot now.
     */
    VoltMod::Result<ActionContext> Resolve(VoltMod::PlayerRef caller, VoltMod::PlayerRef target,
                                           std::string_view permission) const;

    void Run(VoltMod::PlayerRef caller, VoltMod::PlayerRef target, const Action& action) const;
    void Run(VoltMod::PlayerRef caller, VoltMod::PlayerRef target, int param, const ParamAction& action) const;

    /** Announce @p translationKey for @p ctx through @ref OnBroadcast. */
    void Broadcast(const ActionContext& ctx, std::string_view translationKey) const;

private:
    VoltMod::Policy& _policy;
};

}  // namespace AdminSystem::Admin::Actions
