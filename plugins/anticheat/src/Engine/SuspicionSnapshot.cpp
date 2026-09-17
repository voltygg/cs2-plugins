#include "Engine/SuspicionSnapshot.hpp"

#include <VoltMod/Core/Time.hpp>

namespace Anticheat
{

void SuspicionSnapshot::Initialize()
{
    // Disconnected is raised before the slot changes hands, so the evidence is still there to take.
    _subs.Add(_rt.Players.Disconnected += [this](VoltMod::Player& player) { Keep(player); });
    // The slot is value-reset before Connected, so returning it has to wait until after that.
    _subs.Add(_rt.Players.FullyConnected += [this](VoltMod::Player& player) { Return(player); });
}

void SuspicionSnapshot::Keep(VoltMod::Player& player)
{
    const int64_t steamId = player.SteamId();
    if (steamId == 0)
        return;

    const double now = VoltMod::Time::MonotonicSeconds();
    if (_detectors.Scores.Total(player.Slot(), now) < ForgetBelow)
    {
        _held.erase(steamId);
        return;
    }
    _held[steamId] = _detectors.Scores.Save(player.Slot());
}

void SuspicionSnapshot::Return(VoltMod::Player& player)
{
    const auto held = _held.find(player.SteamId());
    if (held == _held.end())
        return;

    _detectors.Scores.Restore(player.Slot(), held->second);
    _held.erase(held);
}

}  // namespace Anticheat
