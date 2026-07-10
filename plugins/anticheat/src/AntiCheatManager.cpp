#include "AntiCheatManager.hpp"

#include "Detectors/AimSnapDetector.hpp"
#include "Detectors/AngleSanityDetector.hpp"
#include "Detectors/NoFlashDetector.hpp"
#include "Detectors/ShotAngleDetector.hpp"
#include "Detectors/SilentAimDetector.hpp"
#include "Detectors/SpinbotDetector.hpp"
#include "Managers.hpp"

#include <CS2Kit/Core/Slot.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <algorithm>
#include <cstdlib>
#include <mathlib/vector.h>

using CS2Kit::Core::Engine;
using CS2Kit::Core::IsValidSlot;
namespace Log = CS2Kit::Utils::Log;
namespace AngleMath = CS2Kit::AngleMath;

namespace Anticheat
{

using CS2Kit::Events::PlayerBlind;
using CS2Kit::Events::PlayerDeath;
using CS2Kit::Events::PlayerHurt;
using CS2Kit::Events::PlayerSpawn;
using CS2Kit::Events::WeaponFire;

namespace
{
constexpr float NoAimError = 1000.0f;

const DetectorSettings& DetectorConfig()
{
    return App().Config.Get().anticheat.detectors;
}
}  // namespace

void AntiCheatManager::Initialize()
{
    Engine().InputHistory.Enable(App().Config.Get().anticheat.historyDepth);
    _players.BindReset();
    _dumpTicks.BindReset();
    _simulator.Initialize();

    // The movement hook needs a live pawn; retry from every spawn until it takes.
    Engine().Events.Listen<PlayerSpawn>([](const PlayerSpawn&) { Engine().MovementHook.Install(); });

    Engine().MovementHook.ListenPreCmd([this](int slot, const CS2Kit::UserCmdView& cmd) { OnCmd(slot, cmd); });
    Engine().Events.Listen<WeaponFire>([this](const WeaponFire& e) { OnWeaponFire(e); });
    Engine().Events.Listen<PlayerHurt>([this](const PlayerHurt& e) { OnPlayerHurt(e); });
    Engine().Events.Listen<PlayerDeath>([this](const PlayerDeath& e) { OnPlayerDeath(e); });
    Engine().Events.Listen<PlayerBlind>([this](const PlayerBlind& e) { OnPlayerBlind(e); });

    _cmdReload.emplace("anticheat_reload", "Re-read settings.jsonc.", [](const CCommand&) {
        if (App().Config.Load(SettingsPath))
            Log::Info("Settings reloaded (mode={}).", App().Config.Get().anticheat.mode);
    });
    _cmdStatus.emplace("anticheat_status", "Dump per-player anticheat scores.",
                       [this](const CCommand&) { _response.DumpStatus(); });

    _cmdDump.emplace("anticheat_dumpcmd", "Log raw usercmds for a slot: anticheat_dumpcmd <slot> [ticks=64]",
                     [this](const CCommand& args) {
                         if (args.ArgC() < 2)
                         {
                             Log::Warn("Usage: anticheat_dumpcmd <slot> [ticks=64]");
                             return;
                         }
                         int slot = std::atoi(args.Arg(1));
                         if (!IsValidSlot(slot))
                         {
                             Log::Warn("anticheat_dumpcmd: '{}' is not a valid slot.", args.Arg(1));
                             return;
                         }
                         _dumpTicks[slot] = args.ArgC() > 2 ? std::atoi(args.Arg(2)) : 64;
                         Log::Info("Dumping {} usercmds for slot {}.", _dumpTicks[slot], slot);
                     });
}

void AntiCheatManager::OnCmd(int slot, const CS2Kit::UserCmdView& cmd)
{
    if (!cmd.Valid || !IsValidSlot(slot))
        return;

    CS2Kit::PlayerController controller(slot);
    if (!controller.IsValid() || (controller.GetFlags() & CS2Kit::Sdk::FL_FAKECLIENT) || !controller.IsAlive())
        return;

    auto& s = _players[slot];
    ++s.Tick;

    if (int& dump = _dumpTicks[slot]; dump > 0)
    {
        --dump;
        Log::Info("[AC dump s{}] yaw={:.1f} pitch={:.1f} mouse=({},{}) subticks={} ihist={} atk1={} shotDiverge={:.1f}",
                  slot, cmd.ViewYaw, cmd.ViewPitch, cmd.MouseDx, cmd.MouseDy, cmd.SubtickMoveCount,
                  cmd.InputHistorySampleCount, cmd.Attack1StartHistoryIndex,
                  Detectors::ShotAngle::ShotDivergence(cmd).value_or(-1.0f));
    }

    float yawDelta = 0.0f;
    float pitchDelta = 0.0f;
    if (s.HasPrevAngles)
    {
        yawDelta = AngleMath::NormalizeAngleDelta(cmd.ViewYaw - s.PrevYaw);
        pitchDelta = AngleMath::NormalizeAngleDelta(cmd.ViewPitch - s.PrevPitch);
    }
    s.PrevYaw = cmd.ViewYaw;
    s.PrevPitch = cmd.ViewPitch;
    s.HasPrevAngles = true;

    const auto& det = DetectorConfig();
    if (auto d = Detectors::AngleSanity::OnCmd(det.sanity, s, cmd.ViewPitch, cmd.ViewYaw))
        _response.Handle(slot, *d);
    if (auto d = Detectors::Spinbot::OnCmd(det.spin, s, Detectors::Spinbot::SpinYawDelta(cmd, yawDelta)))
        _response.Handle(slot, *d);
    Detectors::SilentAim::OnCmd(det.silentAim, s, cmd, yawDelta, pitchDelta);
    Detectors::ShotAngle::OnCmd(det.shotAngle, s, cmd);
}

void AntiCheatManager::OnWeaponFire(const WeaponFire& e)
{
    if (!IsValidSlot(e.Slot))
        return;

    auto& s = _players[e.Slot];
    s.LastFireTick = s.Tick;
    s.HasFired = true;

    QAngle eye = CS2Kit::PlayerController(e.Slot).GetEyeAngles();
    s.FirePitch = eye.x;
    s.FireYaw = eye.y;

    if (auto d = Detectors::AimSnap::OnFire(DetectorConfig().aimSnap, s, e.Slot))
        _response.Handle(e.Slot, *d);
}

float AntiCheatManager::AimErrorDeg(int attackerSlot, int victimSlot, const PlayerState& s) const
{
    CS2Kit::PlayerController attacker(attackerSlot);
    CS2Kit::PlayerController victim(victimSlot);
    if (!attacker.IsValid() || !victim.IsValid())
        return NoAimError;

    // Aim at the moment of the shot; fall back to live eye angles when the
    // damage did not follow a recent weapon_fire (e.g. grenades).
    AngleMath::AimAngles aim;
    if (s.HasFired && s.Tick - s.LastFireTick <= 8)
        aim = {.Pitch = s.FirePitch, .Yaw = s.FireYaw};
    else
    {
        QAngle eye = attacker.GetEyeAngles();
        aim = {.Pitch = eye.x, .Yaw = eye.y};
    }

    Vector eyePos = attacker.GetEyePosition();
    Vector head = victim.GetEyePosition();
    Vector origin = victim.GetAbsOrigin();
    Vector chest = (head + origin) * 0.5f;

    float toHead = AngleMath::AngularDistance(aim, AngleMath::AnglesToPoint(eyePos, head));
    float toChest = AngleMath::AngularDistance(aim, AngleMath::AnglesToPoint(eyePos, chest));
    return std::min(toHead, toChest);
}

void AntiCheatManager::OnPlayerHurt(const PlayerHurt& e)
{
    if (!IsValidSlot(e.AttackerSlot) || !IsValidSlot(e.VictimSlot) || e.AttackerSlot == e.VictimSlot)
        return;

    auto& s = _players[e.AttackerSlot];
    double now = NowSeconds();
    const auto& det = DetectorConfig();
    float aimError = AimErrorDeg(e.AttackerSlot, e.VictimSlot, s);

    if (auto d = Detectors::AimSnap::OnDamage(det.aimSnap, s, aimError, now))
        _response.Handle(e.AttackerSlot, *d);
    if (auto d = Detectors::SilentAim::OnDamage(det.silentAim, s, aimError, now))
        _response.Handle(e.AttackerSlot, *d);
    if (auto d = Detectors::ShotAngle::OnDamage(det.shotAngle, s, now))
        _response.Handle(e.AttackerSlot, *d);
}

void AntiCheatManager::OnPlayerDeath(const PlayerDeath& e)
{
    if (!IsValidSlot(e.AttackerSlot) || !IsValidSlot(e.VictimSlot) || e.AttackerSlot == e.VictimSlot)
        return;

    auto& s = _players[e.AttackerSlot];
    double now = NowSeconds();
    const auto& det = DetectorConfig();
    float aimError = AimErrorDeg(e.AttackerSlot, e.VictimSlot, s);

    if (auto d = Detectors::Spinbot::OnKill(det.spin, s, e.Headshot, aimError, now))
        _response.Handle(e.AttackerSlot, *d);
    if (auto d = Detectors::NoFlash::OnKill(det.noFlash, s, e.Headshot, now))
        _response.Handle(e.AttackerSlot, *d);
}

void AntiCheatManager::OnPlayerBlind(const PlayerBlind& e)
{
    if (!IsValidSlot(e.Slot))
        return;
    Detectors::NoFlash::OnBlind(DetectorConfig().noFlash, _players[e.Slot], e.BlindDuration, NowSeconds());
}

}  // namespace Anticheat
