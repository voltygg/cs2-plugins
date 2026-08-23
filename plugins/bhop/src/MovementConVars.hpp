#pragma once

#include "Config.hpp"

#include <CS2Kit/Api.hpp>
#include <cstddef>
#include <string>
#include <vector>

namespace Bhop
{

/**
 * The set of movement convars the active settings override, plus every operation the two bhop
 * modes perform on them - so BhopManager can stay pure policy (modes, grants, boost).
 *
 * "enabled" mode sets the overrides server-wide through the engine (ApplyGlobal/RestoreGlobal):
 * they replicate to every client and the feature is fully client-predicted. "grants" mode leaves
 * the server untouched - it replicates the values to a single client (ReplicateOverrides) and
 * flips the raw server-side storage only around that player's RunCommand (FlipRaw/RestoreRaw),
 * restoring the real values on revoke (ReplicateServerValues).
 */
class MovementConVars
{
public:
    explicit MovementConVars(CS2Kit::ConVarService& conVars) : _conVars(conVars) {}

    ~MovementConVars() { RestoreGlobal(); }  // unload leaves the server's convars as we found them

    /** (Re)resolve the override set from @p settings; clears any previous set first. */
    void Build(const BhopSettings& settings);

    std::size_t Count() const { return _overrides.size(); }

    /** Undo any global apply and drop the override set (bhop_reload restores before rebuilding). */
    void Reset();

    /** "enabled" mode: save the current server values (once) and set the overrides engine-wide. */
    void ApplyGlobal();

    /** "grants" mode: push the override / current server values to one client's prediction. */
    void ReplicateOverrides(int slot) const;
    void ReplicateServerValues(int slot) const;

    /** "grants" mode: flip raw storage to the bhop values for one player's RunCommand, then back.
     *  RestoreRaw is a no-op unless a FlipRaw is outstanding. */
    void FlipRaw();
    void RestoreRaw();

private:
    /** One convar the active settings override, with everything both modes need. */
    struct ConVarOverride
    {
        const char* Name;
        bool IsFloat;
        float Value;              // bools use 0/1
        std::string NetValue;     // string form sent via ReplicateToClient
        CS2Kit::RawConVar Raw;    // raw storage handle for the per-player flip
        float SavedValue = 0.0f;  // engine value saved before ApplyGlobal / a flip
    };

    void RestoreGlobal();

    CS2Kit::ConVarService& _conVars;
    std::vector<ConVarOverride> _overrides;
    bool _globalApplied = false;
    bool _flipped = false;
};

}  // namespace Bhop
