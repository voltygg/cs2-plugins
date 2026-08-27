#pragma once

#include "Config.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Entities/Api.hpp>
#include <cstddef>
#include <vector>

namespace Bhop
{

/**
 * The set of movement convars the active settings override, plus every operation the two bhop
 * modes perform on them - so BhopManager can stay pure policy (modes, grants, boost).
 *
 * "enabled" mode sets the overrides server-wide through the console (ApplyGlobal): they replicate
 * to every client and the feature is fully client-predicted. "grants" mode leaves the server
 * untouched - it replicates the values to a single client (ReplicateOverrides) and flips the raw
 * server-side storage only around that player's RunCommand (HoldRaw/ReleaseRaw), restoring the
 * real values on revoke (ReplicateServerValues).
 *
 * The convars split by engine type: the two bunnyhop switches are bools and the rest are floats,
 * so each kind gets its own typed handles rather than one untyped table.
 */
class MovementConVars
{
public:
    explicit MovementConVars(VoltMod::ConVars& conVars) : _conVars(conVars) {}

    // _globalOverrides restores whatever ApplyGlobal changed, so unload leaves the
    // server's convars as we found them.
    ~MovementConVars() = default;

    /** (Re)resolve the override set from @p settings; clears any previous set first. */
    void Build(const BhopSettings& settings);

    std::size_t Count() const { return _flags.size() + _numbers.size(); }

    /** Undo any global apply and drop the override set (bhop_reload restores before rebuilding). */
    void Reset();

    /** "enabled" mode: save the current server values (once) and set the overrides engine-wide. */
    void ApplyGlobal();

    /** "grants" mode: push the override / current server values to one client's prediction. */
    void ReplicateOverrides(int slot);
    void ReplicateServerValues(int slot);

    /**
     * "grants" mode: flip raw storage to the bhop values for one player's RunCommand, then back.
     *
     * The scopes are held between the Movement pre and post hooks rather than over a C++ block,
     * because that pair is the window. @ref ReleaseRaw is a no-op unless a hold is outstanding,
     * and @ref Build must not run while one is - it would move the handles the scopes point at.
     */
    void HoldRaw();
    void ReleaseRaw();

private:
    /** One convar the active settings override, with the value both modes push. */
    template <class T>
    struct Override
    {
        VoltMod::ConVar<T> Cvar;
        T Value;
    };

    /** Resolve @p name and record it with @p value, warning when the server does not have it. */
    template <class T>
    void Add(const char* name, T value, std::vector<Override<T>>& into);

    void RestoreGlobal();

    VoltMod::ConVars& _conVars;
    /** The server-wide take-over for "enabled" mode; the saved values and their restore live here.
     *  The raw flips and per-client replication below are bhop-specific and stay put. */
    VoltMod::ConVarOverrides _globalOverrides{_conVars};
    std::vector<Override<bool>> _flags;
    std::vector<Override<float>> _numbers;
    /** Live only between HoldRaw and ReleaseRaw; each one restores its convar when it dies. */
    std::vector<VoltMod::ConVarRawScope<bool>> _flagFlips;
    std::vector<VoltMod::ConVarRawScope<float>> _numberFlips;
};

}  // namespace Bhop
