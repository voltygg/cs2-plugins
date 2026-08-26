#pragma once

#include "Config.hpp"

#include <VoltMod/Api.hpp>
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
    explicit MovementConVars(VoltMod::ConVarService& conVars) : _conVars(conVars) {}

    // _globalLease's destructor restores whatever ApplyGlobal took, so unload leaves the
    // server's convars as we found them.
    ~MovementConVars() = default;

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
        float Value;                     // bools use 0/1
        std::string NetValue;            // string form sent via ReplicateToClient
        VoltMod::ConVarStorage Storage;  // value-storage handle for the per-player flip
        float SavedValue = 0.0f;         // engine value saved before a raw flip
    };

    void RestoreGlobal();

    VoltMod::ConVarService& _conVars;
    /** The server-wide take-over for "enabled" mode; the saved values and their restore live here.
     *  The raw flips and per-client replication below are bhop-specific and stay put. */
    VoltMod::ConVarLease _globalLease{_conVars};
    std::vector<ConVarOverride> _overrides;
    bool _flipped = false;
};

}  // namespace Bhop
