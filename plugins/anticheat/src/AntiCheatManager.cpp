#include "AntiCheatManager.hpp"

#include <VoltMod/Core/Json.hpp>
#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Slot.hpp>
#include <format>
#include <map>
#include <string>
#include <tuple>

using VoltMod::IsValidSlot;

namespace Log = VoltMod::Log;

namespace Anticheat
{

void AntiCheatManager::Initialize()
{
    _dumpTicks.BindReset(_rt.Slots);
    _detectors.Initialize();
    _simulator.Initialize();

    _subs.Add(_rt.Hooks.Movement.Before +=
              [this](int slot, const VoltMod::PlayerInput& cmd) { DumpCommand(slot, cmd); });
    _subs.Add(_rt.Slots.Changed += [this](int slot) { OnSlotChanged(slot); });
    _subs.Add(_rt.Players.FullyConnected += [this](VoltMod::Player& player) { OnPlayerFullyConnected(player); });
    _subs.Add(_rt.Players.SettingsChanged +=
              [this](VoltMod::Player& player) { _namechangerPoll.OnSettingsChanged(player); });
    _subs.Add(_rt.ConVars.Changed += [this](const VoltMod::ConVarChange& change) {
        if (_detectors.OnConVarChanged(change))
            ResetEvidence();
    });

    LoadDetectionData();
    _feed.Initialize();
    _namechangerPoll.Initialize();
    _dllInjection.Initialize();
    _cvarPoll.Initialize();

    _rt.Status.RegisterSection("anticheat", [this] { return StatusSnapshot(); });
    RegisterCommands();
    Log::Info("Detection rules ready (mode={}).", _config.Get().anticheat.mode);
}

void AntiCheatManager::LoadDetectionData()
{
    const DetectionData& data = _detections.Get();
    const std::vector<std::string> rejected = _detectors.InvalidCvars.LoadRules(data.cvarRules);

    if (!rejected.empty())
    {
        std::string names;
        for (const std::string& name : rejected)
            names += (names.empty() ? "" : ", ") + name;
        Log::Warn("Ignoring duplicate cvar rule(s): {}.", names);
    }

    Log::Info("Detection data: {} cvar rule(s), {} blacklisted event(s).", _detectors.InvalidCvars.Rules().Size(),
              data.dllEventBlacklist.size());
}

void AntiCheatManager::DumpCommand(int slot, const VoltMod::PlayerInput& cmd)
{
    if (!cmd.Valid || !IsValidSlot(slot))
        return;
    int& remaining = _dumpTicks[slot];
    if (remaining <= 0)
        return;
    --remaining;

    // Show whether the attack sample was present, capped, or absent.
    const int attackIndex = cmd.Attack1StartHistoryIndex;
    std::string attack = "none";
    if (attackIndex >= 0)
    {
        if (auto sample = cmd.SampleAt(attackIndex))
            attack = sample->HasViewAngles
                         ? std::format("[{}] pitch={:.2f} yaw={:.2f}", attackIndex, sample->ViewPitch, sample->ViewYaw)
                         : std::format("[{}] no angles", attackIndex);
        else
            attack = std::format("[{}] {}", attackIndex,
                                 attackIndex >= cmd.InputHistoryTotalCount ? "out of range" : "capped away");
    }

    float subtickPitch = 0.0f;
    float subtickYaw = 0.0f;
    for (int i = 0; i < cmd.SubtickMoveCount; ++i)
    {
        subtickPitch += cmd.SubtickMoves[i].PitchDelta;
        subtickYaw += cmd.SubtickMoves[i].YawDelta;
    }

    std::string punch = "none";
    if (const VoltMod::Pawn pawn = _rt.Entities.PawnOf(slot))
    {
        if (const auto services = pawn.AimPunchServices())
        {
            const QAngle base = services.BaseAngle();
            punch = std::format("({:.3f},{:.3f}) tick={}", base.x, base.y, services.BaseTick());
        }
    }

    Log::Info(
        "[AC dump s{}] cmd={} clientTick={} view=({:.2f},{:.2f},{:.2f}) mouse=({},{}) buttons={:#x}/{:#x} "
        "subticks={} (dPitch={:.3f} dYaw={:.3f}) history={}/{} attack1={} punch={}",
        slot, cmd.CommandNumber, cmd.ClientTick, cmd.ViewPitch, cmd.ViewYaw, cmd.ViewRoll, cmd.MouseDx, cmd.MouseDy,
        cmd.ButtonsHeld, cmd.ButtonsChanged, cmd.SubtickMoveCount, subtickPitch, subtickYaw,
        cmd.InputHistorySampleCount, cmd.InputHistoryTotalCount, attack, punch);
}

std::string AntiCheatManager::StatusSnapshot() const
{
    const auto& settings = _config.Get().anticheat;
    std::map<std::string, bool> modules;
    for (const DetectionInfo& detection : DetectionCatalog)
        modules.emplace(detection.Token, _detectors.RuleEnabled(detection.Kind));

    const VoltMod::Status sight = _feed.SightAvailable();
    return VoltMod::Json::Write(
        glz::obj{"enabled",
                 settings.enabled,
                 "mode",
                 ModeName(_response.CurrentMode()),
                 "detecting",
                 _detectors.Enabled(),
                 "enforcingCheatCvars",
                 _detectors.EnforceCheatCvars(),
                 "modules",
                 modules,
                 "clientCvars",
                 _rt.Hooks.ClientConVars.Available() ? "available" : "degraded",
                 "teleportTracker",
                 _rt.Hooks.Teleport.Available().has_value(),
                 "sightLines",
                 sight ? std::string("available") : sight.error().Detail,
                 "correlatorFrames",
                 _detectors.History.FrameCount(),
                 "detectionData",
                 glz::obj{"cvarRules", _detectors.InvalidCvars.Rules().Size(), "blacklistedEvents",
                          _detections.Get().dllEventBlacklist.size()},
                 "webhook",
                 !settings.webhook.url.empty(),
                 "simulator",
                 settings.debug.simulator,
                 "includeBots",
                 settings.debug.includeBots});
}

void AntiCheatManager::ResetEvidence()
{
    std::apply([](auto&... modules) { (modules.Reset(), ...); }, Modules());
}

void AntiCheatManager::OnMapStart()
{
    _detectors.RefreshTeamRules();
    ResetEvidence();
}

void AntiCheatManager::OnSlotChanged(int slot)
{
    std::apply([slot](auto&... modules) { (modules.OnSlotChanged(slot), ...); }, Modules());
}

void AntiCheatManager::OnPlayerFullyConnected(VoltMod::Player& player)
{
    _namechangerPoll.OnFullyConnected(player);
    _dllInjection.OnFullyConnected(player.Slot());
    _cvarPoll.OnFullyConnected(player.Slot());
}

}  // namespace Anticheat
