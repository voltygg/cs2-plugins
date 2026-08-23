#include "AntiCheatManager.hpp"

#include "Managers.hpp"

#include <CS2Kit/Core/Slot.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <cstdlib>
#include <format>
#include <string>
#include <string_view>

using CS2Kit::App::Engine;
using CS2Kit::Core::IsValidSlot;
namespace Log = CS2Kit::Utils::Log;

namespace Anticheat
{

using CS2Kit::Events::PlayerSpawn;

void AntiCheatManager::Initialize()
{
    _dumpTicks.BindReset();
    _simulator.Initialize();

    // The teleport tracker binds itself from its own spawn listener, so it must be enabled here:
    // registering a listener while the event service dispatches would mutate the map it walks.
    Engine().Sdk.Teleports.Enable();

    // The movement hook needs a live movement-services instance, so retry from every spawn until it
    // takes. Install() touches no listener registry.
    Engine().Sdk.Events.Listen<PlayerSpawn>([](const PlayerSpawn&) { Engine().Sdk.MovementHook.Install(); });

    Engine().Sdk.MovementHook.ListenPreCmd([this](int slot, const CS2Kit::UserCmdView& cmd) { DumpCommand(slot, cmd); });
    Engine().Players.ListenSlotChange([this](int slot) { OnSlotChanged(slot); });

    // sv_cheats going off starts a propagation grace before client values mean anything again.
    Engine().Sdk.ConVars.OnChange([this](const char* name, const char*, const char* newValue) {
        const std::string_view changed = name ? name : "";
        if (changed == "mp_teammates_are_enemies")
        {
            RefreshTeamRules();
            ResetEvidence();
            return;
        }
        if (changed != "sv_cheats")
            return;
        const bool enabled = newValue && std::string_view(newValue) != "0" && std::string_view(newValue) != "false";
        if (!enabled)
            _cheatGraceUntil = TimeUtils::MonotonicSeconds() + SvCheatsPropagationGraceSec;
        ResetEvidence();
    });
    _cheatGraceUntil = TimeUtils::MonotonicSeconds() + SvCheatsPropagationGraceSec;

    RefreshTeamRules();
    LoadDetectionData();
    _feed.Initialize();
    _dllInjection.Initialize();
    _invalidCvarPoller.Initialize();

    Engine().Status.RegisterSection("anticheat", [this] { return StatusSnapshot(); });
    RegisterCommands();
    Log::Info("Detection cores ready (mode={}).", App().Config.Get().anticheat.mode);
}

void AntiCheatManager::RefreshTeamRules()
{
    _correlator.SetTeammatesAreEnemies(Engine().Sdk.ConVars.GetBool("mp_teammates_are_enemies").value_or(false));
}

void AntiCheatManager::LoadDetectionData()
{
    const DetectionData& data = App().Detections.Get();
    const std::vector<std::string> rejected = _invalidCvars.LoadRules(data.cvarRules);

    // Naming them: the operator editing this file is not the one who can read the source.
    if (!rejected.empty())
    {
        std::string names;
        for (const std::string& name : rejected)
            names += (names.empty() ? "" : ", ") + name;
        Log::Warn("Ignoring duplicate cvar rule(s): {}.", names);
    }

    // A count of zero here is the whole diagnostic for a module that is silently doing nothing.
    Log::Info("Detection data: {} cvar rule(s), {} blacklisted event(s).", _invalidCvars.Rules().Size(),
              data.dllEventBlacklist.size());
}

void AntiCheatManager::RegisterCommands()
{
    _cmdReload.emplace(
        "anticheat_reload", "Re-read settings.jsonc and detections.jsonc, and drop all accumulated evidence.",
        [this](const CCommand&) {
            if (!App().Config.Load(CS2Kit::AddonFile(AddonName, "configs/settings.jsonc")))
                return;
            // Keeps the rules already in memory when the edit does not parse, so a typo cannot
            // silently disarm the two table-driven modules.
            if (!App().Detections.Load(DetectionDataPath))
                Log::Warn("{} could not be re-read; keeping the tables already loaded.", DetectionDataPath);
            else
                LoadDetectionData();
            RefreshTeamRules();
            ResetEvidence();
            Log::Info("Settings reloaded (mode={}); evidence cleared.", App().Config.Get().anticheat.mode);
        });

    _cmdStatus.emplace("anticheat_status", "Print the module state and per-player detection evidence.",
                       [this](const CCommand&) { LogStatus(); });

    _cmdDump.emplace("anticheat_dumpcmd", "Log raw usercmds for a slot: anticheat_dumpcmd <slot> [ticks=64]",
                     [this](const CCommand& args) {
                         if (args.ArgC() < 2)
                         {
                             Log::Warn("Usage: anticheat_dumpcmd <slot> [ticks=64]");
                             return;
                         }
                         const int slot = std::atoi(args.Arg(1));
                         if (!IsValidSlot(slot))
                         {
                             Log::Warn("anticheat_dumpcmd: '{}' is not a valid slot.", args.Arg(1));
                             return;
                         }
                         _dumpTicks[slot] = args.ArgC() > 2 ? std::atoi(args.Arg(2)) : 64;
                         Log::Info("Dumping {} usercmds for slot {}.", _dumpTicks[slot], slot);
                     });
}

void AntiCheatManager::DumpCommand(int slot, const CS2Kit::UserCmdView& cmd)
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
        if (const CS2Kit::InputHistorySample* sample = cmd.SampleAt(attackIndex))
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
    const auto& settings = App().Config.Get().anticheat;

    nlohmann::json modules = nlohmann::json::object();
    for (const DetectionInfo& detection : DetectionCatalog)
        modules[detection.Token] = ModuleEnabled(detection.Kind);

    return nlohmann::json{
        {"enabled", settings.enabled},
        {"mode", ModeName(_response.CurrentMode())},
        {"detecting", DetectionsEnabled()},
        {"enforcingCheatCvars", EnforceCheatCvars()},
        {"modules", std::move(modules)},
        {"clientCvars", Engine().Sdk.ClientCvars.Available() ? "available" : "degraded"},
        {"teleportTracker", Engine().Sdk.Teleports.Enabled()},
        {"correlatorFrames", _correlator.FrameCount()},
        // A module with an empty table is inert however its toggle reads, so report the tables the
        // way clientCvars availability is reported: a health check must be able to see it.
        {"detectionData",
         {
             {"cvarRules", _invalidCvars.Rules().Size()},
             {"blacklistedEvents", App().Detections.Get().dllEventBlacklist.size()},
         }},
        {"webhook", !settings.webhook.url.empty()},
        {"simulator", settings.debug.simulator},
    };
}

void AntiCheatManager::LogStatus() const
{
    Log::Info("[AC] {}", StatusSnapshot().dump());

    const double now = TimeUtils::MonotonicSeconds();
    bool any = false;
    for (const CS2Kit::Players::Player* player : Engine().Players.GetAllPlayers())
    {
        const int slot = player ? player->GetSlot() : -1;
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
            slot, player->GetName(), player->GetSteamID(), PunishmentName(_response.Issued(slot)),
            _aimbot.IncidentCount(slot), _aimlock.IncidentCount(slot), _aimlock.IsTracking(slot) ? "/tracking" : "",
            _antiAim.Score(slot), _silentAim.Score(slot, now), _namechanger.ChangeCount(slot),
            latched.empty() ? "-" : latched, Engine().Sdk.ClientCvars.PendingCount(slot),
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

void AntiCheatManager::OnPlayerFullyConnected(CS2Kit::Players::Player* player)
{
    if (!player)
        return;
    _namechangerDetector.OnFullyConnected(player);
    _dllInjection.OnFullyConnected(player->GetSlot());
    _invalidCvarPoller.OnFullyConnected(player->GetSlot());
}

void AntiCheatManager::OnPlayerSettingsChanged(CS2Kit::Players::Player* player)
{
    _namechangerDetector.OnSettingsChanged(player);
}

CS2Kit::RawConVar& AntiCheatManager::CheatsConVar() const
{
    if (!_svCheats)
        _svCheats = Engine().Sdk.ConVars.Raw("sv_cheats");
    return *_svCheats;
}

bool AntiCheatManager::DetectionsEnabled() const
{
    const auto& settings = App().Config.Get().anticheat;
    if (!settings.enabled)
        return false;
    const CS2Kit::RawConVar& cheats = CheatsConVar();
    if (!cheats.Valid())
        return true;
    return !cheats.GetBool() || settings.allowSvCheatsTesting;
}

bool AntiCheatManager::EnforceCheatCvars() const
{
    const CS2Kit::RawConVar& cheats = CheatsConVar();
    return ShouldEnforceCheatCvars(cheats.Valid() && cheats.GetBool(), TimeUtils::MonotonicSeconds(), _cheatGraceUntil);
}

bool AntiCheatManager::ModuleEnabled(DetectionKind kind)
{
    return DetectionEnabled(App().Config.Get().anticheat.detections, kind);
}

bool AntiCheatManager::IsEligible(int slot)
{
    if (!IsValidSlot(slot))
        return false;
    // The pawn flag is unreadable before a player has a pawn, so identity decides it first.
    const CS2Kit::Players::Player* player = Engine().Players.GetPlayerBySlot(slot);
    if (!player || player->IsBot())
        return false;
    CS2Kit::PlayerController controller(slot);
    return controller.IsValid() && !(controller.GetFlags() & CS2Kit::Sdk::FL_FAKECLIENT);
}

void AntiCheatManager::Report(int slot, const std::optional<Finding>& finding)
{
    if (finding)
        _response.Handle(slot, *finding);
}

}  // namespace Anticheat
