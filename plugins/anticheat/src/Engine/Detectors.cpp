#include "Engine/Detectors.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Slot.hpp>
#include <VoltMod/Core/Time.hpp>

namespace Log = VoltMod::Log;

namespace Anticheat
{

using VoltMod::Time;
using Rules::ShouldEnforceCheatCvars;
using Rules::SvCheatsPropagationGraceSec;

void Detectors::Initialize()
{
    Scores.Configure({});

    _cheatGraceUntil = Time::MonotonicSeconds() + SvCheatsPropagationGraceSec;

    if (auto cheats = _rt.ConVars.Find<bool>("sv_cheats"))
        _svCheats = std::move(*cheats);
    else
        Log::Warn("sv_cheats unusable ({}); detections run as if it were off.", cheats.error().Detail);
    if (auto freeForAll = _rt.ConVars.Find<bool>("mp_teammates_are_enemies"))
        _teammatesAreEnemies = std::move(*freeForAll);
    else
        Log::Warn("mp_teammates_are_enemies unusable ({}); normal team rules apply.", freeForAll.error().Detail);

    RefreshTeamRules();
}

bool Detectors::Enabled() const
{
    const auto& settings = _config.Get().anticheat;
    if (!settings.enabled)
        return false;
    if (!_svCheats)
        return true;
    return !_svCheats.Get() || settings.allowSvCheatsTesting;
}

bool Detectors::RuleEnabled(DetectionKind kind) const
{
    return DetectionEnabled(_config.Get().anticheat.detections, kind);
}

bool Detectors::IncludesBots() const
{
    return _config.Get().anticheat.debug.includeBots;
}

bool Detectors::IsEligible(int slot)
{
    if (!VoltMod::IsValidSlot(slot))
        return false;
    const VoltMod::Player* player = _rt.Players.Get(slot);
    if (!player)
        return false;
    // FL_FAKECLIENT lives on the pawn, so a slot that has not spawned yet cannot be cleared.
    VoltMod::Pawn pawn = _rt.Entities.PawnOf(slot);
    if (!pawn)
        return false;
    const bool bot = player->IsBot() || (pawn.Flags() & VoltMod::FL_FAKECLIENT);
    return !bot || IncludesBots();
}

void Detectors::Report(int slot, const std::optional<Finding>& finding)
{
    if (finding)
        _response.Handle(slot, *finding);
}

bool Detectors::EnforceCheatCvars() const
{
    return ShouldEnforceCheatCvars(_svCheats && _svCheats.Get(), Time::MonotonicSeconds(), _cheatGraceUntil);
}

bool Detectors::OnConVarChanged(const VoltMod::ConVarChange& change)
{
    if (change.Name == "mp_teammates_are_enemies")
    {
        RefreshTeamRules();
        return true;
    }
    if (change.Name != "sv_cheats")
        return false;
    // Allow client values to settle after sv_cheats is disabled.
    const bool enabled = !change.NewValue.empty() && change.NewValue != "0" && change.NewValue != "false";
    if (!enabled)
        _cheatGraceUntil = Time::MonotonicSeconds() + SvCheatsPropagationGraceSec;
    return true;
}

void Detectors::RefreshTeamRules()
{
    History.SetTeammatesAreEnemies(_teammatesAreEnemies && _teammatesAreEnemies.Get());
}

void Detectors::Reset()
{
    std::apply([](auto&... detectors) { (detectors.Reset(), ...); }, All());
}

void Detectors::OnSlotChanged(int slot)
{
    std::apply([slot](auto&... detectors) { (detectors.OnSlotChanged(slot), ...); }, All());
}

}  // namespace Anticheat
