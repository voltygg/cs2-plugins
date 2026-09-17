#pragma once

#include "Engine/Detectors.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Subscriptions.hpp>
#include <cstdint>
#include <unordered_map>

namespace Anticheat
{

/**
 * Holds a player's suspicion between their sessions, keyed by SteamID.
 *
 * Without it, evidence lives only as long as a slot, so reconnecting hands a cheat a clean slate.
 * Entries are dropped once they decay to nothing, so this grows with players still under suspicion
 * rather than with every player the server has ever seen.
 */
class SuspicionSnapshot
{
public:
    SuspicionSnapshot(Detectors& detectors, VoltMod::Runtime& runtime) : _detectors(detectors), _rt(runtime) {}

    void Initialize();

    /** Forget everyone, for the operator reset that also clears live evidence. */
    void Reset() { _held.clear(); }

private:
    /** Below this much suspicion a player is not worth remembering between sessions. */
    static constexpr float ForgetBelow = 0.05f;

    void Keep(VoltMod::Player& player);
    void Return(VoltMod::Player& player);

    Detectors& _detectors;
    VoltMod::Runtime& _rt;
    std::unordered_map<int64_t, PlayerEvidence> _held;
    VoltMod::Subscriptions _subs;
};

}  // namespace Anticheat
