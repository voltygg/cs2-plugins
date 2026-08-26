#include "AntiCheatManager.hpp"

#include "App.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Slot.hpp>
#include <format>
#include <optional>
#include <string>

using VoltMod::Caller;
using VoltMod::IsValidSlot;
using VoltMod::Reply;
using VoltMod::Result;
using VoltMod::Time;

namespace Args = VoltMod::Args;
namespace Log = VoltMod::Log;

namespace Anticheat
{

static constexpr int DefaultDumpTicks = 64;
static constexpr int MaxDumpTicks = 10000;

void AntiCheatManager::Initialize()
{
    _dumpTicks.BindReset(_rt.Slots);
    _simulator.Initialize();

    // Subscribing installs the movement hook, which every aim module then feeds off.
    _subs.push_back(_rt.Hooks.Movement.PreCmd +=
                    [this](int slot, const VoltMod::UserCmdView& cmd) { DumpCommand(slot, cmd); });
    _subs.push_back(_rt.Slots.Changed += [this](int slot) { OnSlotChanged(slot); });
    _subs.push_back(_rt.Players.FullyConnected += [this](VoltMod::Player& player) { OnPlayerFullyConnected(player); });
    _subs.push_back(_rt.Players.SettingsChanged +=
                    [this](VoltMod::Player& player) { OnPlayerSettingsChanged(player); });

    // sv_cheats going off starts a propagation grace before client values mean anything again.
    _subs.push_back(_rt.ConVars.Changed += [this](const VoltMod::ConVarChange& change) {
        if (change.Name == "mp_teammates_are_enemies")
        {
            RefreshTeamRules();
            ResetEvidence();
            return;
        }
        if (change.Name != "sv_cheats")
            return;
        const bool enabled = !change.NewValue.empty() && change.NewValue != "0" && change.NewValue != "false";
        if (!enabled)
            _cheatGraceUntil = Time::MonotonicSeconds() + SvCheatsPropagationGraceSec;
        ResetEvidence();
    });
    _cheatGraceUntil = Time::MonotonicSeconds() + SvCheatsPropagationGraceSec;

    RefreshTeamRules();
    LoadDetectionData();
    _feed.Initialize();
    _dllInjection.Initialize();
    _invalidCvarPoller.Initialize();

    _rt.Status.RegisterSection("anticheat", [this] { return StatusSnapshot().dump(); });
    RegisterCommands();
    Log::Info("Detection cores ready (mode={}).", _config.Get().anticheat.mode);
}

void AntiCheatManager::RefreshTeamRules()
{
    auto freeForAll = VoltMod::ConVar<bool>::Find(_rt.ConVars, "mp_teammates_are_enemies");
    _correlator.SetTeammatesAreEnemies(freeForAll && freeForAll->Get());
}

void AntiCheatManager::LoadDetectionData()
{
    const DetectionData& data = _detections.Get();
    const std::vector<std::string> rejected = _invalidCvars.LoadRules(data.cvarRules);

    // Log rejected rule names for the operator who edited the table.
    if (!rejected.empty())
    {
        std::string names;
        for (const std::string& name : rejected)
            names += (names.empty() ? "" : ", ") + name;
        Log::Warn("Ignoring duplicate cvar rule(s): {}.", names);
    }

    // Zero identifies a table-driven detector with no active data.
    Log::Info("Detection data: {} cvar rule(s), {} blacklisted event(s).", _invalidCvars.Rules().Size(),
              data.dllEventBlacklist.size());
}

void AntiCheatManager::RegisterCommands()
{
    auto& commands = _rt.Commands;

    _subs.push_back(commands.Add("anticheat_reload")
                        .Describe("Re-read settings.jsonc and detections.jsonc, and drop all accumulated evidence.")
                        .ConsoleOnly()
                        .Run([this](Caller) -> Result<Reply> {
                            if (!_config.Load(VoltMod::AddonFile(AddonName, "configs/settings.jsonc")))
                                return Reply::Silent();
                            // Keeps the rules already in memory when the edit does not parse, so a typo cannot
                            // silently disarm the two table-driven modules.
                            if (!_detections.Load(DetectionDataPath))
                                Log::Warn("{} could not be re-read; keeping the tables already loaded.",
                                          DetectionDataPath);
                            else
                                LoadDetectionData();
                            RefreshTeamRules();
                            ResetEvidence();
                            return Reply{std::format("Settings reloaded (mode={}); evidence cleared.",
                                                     _config.Get().anticheat.mode)};
                        }));

    _subs.push_back(commands.Add("anticheat_status")
                        .Describe("Print the module state and per-player detection evidence.")
                        .ConsoleOnly()
                        .Run([this](Caller) -> Result<Reply> {
                            LogStatus();
                            return Reply::Silent();
                        }));

    // Two integers, two parameters: the tick count no longer has to ride in a Word.
    _subs.push_back(commands.Add("anticheat_dumpcmd")
                        .Describe("Log raw usercmds for a slot.")
                        .ConsoleOnly()
                        .Run([this](Caller, Args::Int slot, Args::Opt<Args::Int> requested) -> Result<Reply> {
                            if (!IsValidSlot(slot.Value))
                                return Reply{std::format("anticheat_dumpcmd: {} is not a valid slot.", slot.Value)};

                            const int ticks = requested.Value ? requested.Value->Value : DefaultDumpTicks;
                            if (ticks < 1 || ticks > MaxDumpTicks)
                                return Reply{std::format("anticheat_dumpcmd: ticks must be 1-{}.", MaxDumpTicks)};

                            _dumpTicks[slot.Value] = ticks;
                            return Reply{std::format("Dumping {} usercmds for slot {}.", ticks, slot.Value)};
                        }));
}

void AntiCheatManager::DumpCommand(int slot, const VoltMod::UserCmdView& cmd)
{
    if (!cmd.Valid || !IsValidSlot(slot))
        return;
    int& remaining = _dumpTicks[slot];
    if (remaining <= 0)
        return;
    --remaining;

    // The aim modules judge the attack angles, so show what the index resolved to: present, capped
    // away by the history limit, or never sent at all.
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

    Log::Info(
        "[AC dump s{}] cmd={} clientTick={} view=({:.2f},{:.2f},{:.2f}) mouse=({},{}) buttons={:#x}/{:#x} "
        "subticks={} (dPitch={:.3f} dYaw={:.3f}) history={}/{} attack1={}",
        slot, cmd.CommandNumber, cmd.ClientTick, cmd.ViewPitch, cmd.ViewYaw, cmd.ViewRoll, cmd.MouseDx, cmd.MouseDy,
        cmd.ButtonsHeld, cmd.ButtonsChanged, cmd.SubtickMoveCount, subtickPitch, subtickYaw,
        cmd.InputHistorySampleCount, cmd.InputHistoryTotalCount, attack);
}

nlohmann::json AntiCheatManager::StatusSnapshot() const
{
    const auto& settings = _config.Get().anticheat;

    nlohmann::json modules = nlohmann::json::object();
    for (const DetectionInfo& detection : DetectionCatalog)
        modules[detection.Token] = ModuleEnabled(detection.Kind);

    return nlohmann::json{
        {"enabled", settings.enabled},
        {"mode", ModeName(_response.CurrentMode())},
        {"detecting", DetectionsEnabled()},
        {"enforcingCheatCvars", EnforceCheatCvars()},
        {"modules", std::move(modules)},
        {"clientCvars", _rt.Capabilities.Has(VoltMod::Capability::ClientCvars) ? "available" : "degraded"},
        {"teleportTracker", _rt.Capabilities.Has(VoltMod::Capability::Teleport)},
        {"correlatorFrames", _correlator.FrameCount()},
        // A module with an empty table is inert however its toggle reads, so report the tables the
        // way clientCvars availability is reported: a health check must be able to see it.
        {"detectionData",
         {
             {"cvarRules", _invalidCvars.Rules().Size()},
             {"blacklistedEvents", _detections.Get().dllEventBlacklist.size()},
         }},
        {"webhook", !settings.webhook.url.empty()},
        {"simulator", settings.debug.simulator},
    };
}

void AntiCheatManager::LogStatus() const
{
    Log::Info("[AC] {}", StatusSnapshot().dump());

    const double now = Time::MonotonicSeconds();
    bool any = false;
    for (const VoltMod::Player* player : _rt.Players.All())
    {
        const int slot = player ? player->Slot() : -1;
        if (!InSlotRange(slot) || player->IsBot())
            continue;
        any = true;

        std::string latched;
        const std::span<const CvarRule> rules = _invalidCvars.Rules().All();
        for (size_t index = 0; index < rules.size(); ++index)
        {
            if (!_invalidCvars.IsLatchedAt(slot, index))
                continue;
            if (!latched.empty())
                latched += ",";
            latched += rules[index].name;
        }

        Log::Info(
            "[AC] s{} {} ({}) punished={} aimbot={} aimlock={}{} antiaim={:.1f} silentaim={} names={} "
            "cvars=[{}] pending={} poll={:.1f}s shots={} cmds={} gen={}",
            slot, player->Name(), player->SteamId(), PunishmentName(_response.Issued(slot)),
            _aimbot.IncidentCount(slot), _aimlock.IncidentCount(slot), _aimlock.IsTracking(slot) ? "/tracking" : "",
            _antiAim.Score(slot), _silentAim.Score(slot, now), _namechanger.ChangeCount(slot),
            latched.empty() ? "-" : latched, _rt.Hooks.ClientCvars.PendingCount(slot),
            _invalidCvarPoller.PollsIn(slot, now), _correlator.Shots(slot).size(), _correlator.CommandCount(slot),
            _correlator.Generation(slot));
    }
    if (!any)
        Log::Info("[AC] no human players connected.");
}

void AntiCheatManager::ResetEvidence()
{
    std::apply([](auto&... modules) { (modules.Reset(), ...); }, ResettableModules());
}

void AntiCheatManager::OnMapStart()
{
    RefreshTeamRules();
    ResetEvidence();
}

void AntiCheatManager::OnSlotChanged(int slot)
{
    std::apply([slot](auto&... modules) { (modules.OnSlotChanged(slot), ...); }, ResettableModules());
}

void AntiCheatManager::OnPlayerFullyConnected(VoltMod::Player& player)
{
    _namechangerDetector.OnFullyConnected(player);
    _dllInjection.OnFullyConnected(player.Slot());
    _invalidCvarPoller.OnFullyConnected(player.Slot());
}

void AntiCheatManager::OnPlayerSettingsChanged(VoltMod::Player& player)
{
    _namechangerDetector.OnSettingsChanged(player);
}

const VoltMod::ConVar<bool>& AntiCheatManager::CheatsConVar() const
{
    if (!_svCheatsResolved)
    {
        _svCheatsResolved = true;
        if (auto cheats = VoltMod::ConVar<bool>::Find(_rt.ConVars, "sv_cheats"))
            _svCheats = std::move(*cheats);
        else
            Log::Warn("sv_cheats unusable ({}); detections run as if it were off.", cheats.error().Detail);
    }
    return _svCheats;
}

bool AntiCheatManager::DetectionsEnabled() const
{
    const auto& settings = _config.Get().anticheat;
    if (!settings.enabled)
        return false;
    const VoltMod::ConVar<bool>& cheats = CheatsConVar();
    if (!cheats.IsValid())
        return true;
    return !cheats.Get() || settings.allowSvCheatsTesting;
}

bool AntiCheatManager::EnforceCheatCvars() const
{
    const VoltMod::ConVar<bool>& cheats = CheatsConVar();
    return ShouldEnforceCheatCvars(cheats.IsValid() && cheats.Get(), Time::MonotonicSeconds(), _cheatGraceUntil);
}

bool AntiCheatManager::ModuleEnabled(DetectionKind kind) const
{
    return DetectionEnabled(_config.Get().anticheat.detections, kind);
}

bool AntiCheatManager::IsEligible(int slot)
{
    if (!IsValidSlot(slot))
        return false;
    // The pawn flag is unreadable before a player has a pawn, so identity decides it first.
    const VoltMod::Player* player = _rt.Players.Get(slot);
    if (!player || player->IsBot())
        return false;
    VoltMod::Pawn pawn = _rt.Entities.PawnOf(slot);
    return static_cast<bool>(pawn) && !(pawn.Flags.Get() & VoltMod::FL_FAKECLIENT);
}

void AntiCheatManager::Report(int slot, const std::optional<Finding>& finding)
{
    if (finding)
        _response.Handle(slot, *finding);
}

}  // namespace Anticheat
