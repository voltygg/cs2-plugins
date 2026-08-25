#include "AntiCheatManager.hpp"

#include "App.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Slot.hpp>
#include <charconv>
#include <format>
#include <optional>
#include <string>
#include <string_view>

using VoltMod::Core::IsValidSlot;
namespace Log = VoltMod::Core::Log;

namespace Anticheat
{

namespace
{
constexpr int DefaultDumpTicks = 64;
constexpr int MaxDumpTicks = 10000;

std::optional<int> ParseInt(std::string_view text)
{
    int value{};
    const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || ptr != text.data() + text.size())
        return std::nullopt;
    return value;
}
}  // namespace

void AntiCheatManager::Initialize()
{
    _dumpTicks.BindReset();
    _simulator.Initialize();

    // The teleport tracker binds itself from its own spawn listener, so it must be enabled here:
    // registering a listener while the event service dispatches would mutate the map it walks.
    _rt.Teleports.Enable();

    _rt.MovementHook.Install();
    _subs.push_back(
        _rt.MovementHook.ListenPreCmd([this](int slot, const VoltMod::UserCmdView& cmd) { DumpCommand(slot, cmd); }));
    _subs.push_back(_rt.Players.ListenSlotChange([this](int slot) { OnSlotChanged(slot); }));

    // sv_cheats going off starts a propagation grace before client values mean anything again.
    _subs.push_back(_rt.ConVars.OnChange([this](const char* name, const char*, const char* newValue) {
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
    }));
    _cheatGraceUntil = TimeUtils::MonotonicSeconds() + SvCheatsPropagationGraceSec;

    RefreshTeamRules();
    LoadDetectionData();
    _feed.Initialize();
    _dllInjection.Initialize();
    _invalidCvarPoller.Initialize();

    _rt.Status.RegisterSection("anticheat", [this] { return StatusSnapshot(); });
    RegisterCommands();
    Log::Info("Detection cores ready (mode={}).", _config.Get().anticheat.mode);
}

void AntiCheatManager::RefreshTeamRules()
{
    _correlator.SetTeammatesAreEnemies(_rt.ConVars.GetBool("mp_teammates_are_enemies").value_or(false));
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
    using namespace VoltMod::Commands;
    auto& commands = _rt.Commands;

    commands.Register({
        .Name = "anticheat_reload",
        .Description = "Re-read settings.jsonc and detections.jsonc, and drop all accumulated evidence.",
        .Surfaces = Surface::Console,
        .Handler =
            [this](CommandContext&) {
                if (!_config.Load(VoltMod::AddonFile(AddonName, "configs/settings.jsonc")))
                    return CommandResult::Silent();
                // Keeps the rules already in memory when the edit does not parse, so a typo cannot
                // silently disarm the two table-driven modules.
                if (!_detections.Load(DetectionDataPath))
                    Log::Warn("{} could not be re-read; keeping the tables already loaded.", DetectionDataPath);
                else
                    LoadDetectionData();
                RefreshTeamRules();
                ResetEvidence();
                return CommandResult{
                    std::format("Settings reloaded (mode={}); evidence cleared.", _config.Get().anticheat.mode)};
            },
    });

    commands.Register({
        .Name = "anticheat_status",
        .Description = "Print the module state and per-player detection evidence.",
        .Surfaces = Surface::Console,
        .Handler =
            [this](CommandContext&) {
                LogStatus();
                return CommandResult::Silent();
            },
    });

    // The tick count rides in a Word because CommandContext carries one Int; the slot still gets
    // arity checking, cmd.badNumber and the derived console registration from the spec.
    commands.Register({
        .Name = "anticheat_dumpcmd",
        .Description = "Log raw usercmds for a slot.",
        .Usage = "anticheat_dumpcmd <slot> [ticks=64]",
        .Args = {Int(), Word(false)},
        .Surfaces = Surface::Console,
        .Handler =
            [this](CommandContext& c) {
                const int slot = c.Int().value_or(-1);
                if (!IsValidSlot(slot))
                    return CommandResult{std::format("anticheat_dumpcmd: {} is not a valid slot.", slot)};

                int ticks = DefaultDumpTicks;
                if (!c.Word.empty())
                {
                    // Reject non-numeric input instead of treating it as zero ticks.
                    const auto requested = ParseInt(c.Word);
                    if (!requested || *requested < 1 || *requested > MaxDumpTicks)
                        return CommandResult{std::format("anticheat_dumpcmd: ticks must be 1-{}.", MaxDumpTicks)};
                    ticks = *requested;
                }

                _dumpTicks[slot] = ticks;
                return CommandResult{std::format("Dumping {} usercmds for slot {}.", ticks, slot)};
            },
    });
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
        if (const VoltMod::InputHistorySample* sample = cmd.SampleAt(attackIndex))
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
        {"clientCvars", _rt.ClientCvars.Available() ? "available" : "degraded"},
        {"teleportTracker", _rt.Teleports.Enabled()},
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

    const double now = TimeUtils::MonotonicSeconds();
    bool any = false;
    for (const VoltMod::Players::Player* player : _rt.Players.GetAllPlayers())
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
            latched.empty() ? "-" : latched, _rt.ClientCvars.PendingCount(slot), _invalidCvarPoller.PollsIn(slot, now),
            _correlator.Shots(slot).size(), _correlator.CommandCount(slot), _correlator.Generation(slot));
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

void AntiCheatManager::OnPlayerFullyConnected(VoltMod::Players::Player* player)
{
    if (!player)
        return;
    _namechangerDetector.OnFullyConnected(player);
    _dllInjection.OnFullyConnected(player->GetSlot());
    _invalidCvarPoller.OnFullyConnected(player->GetSlot());
}

void AntiCheatManager::OnPlayerSettingsChanged(VoltMod::Players::Player* player)
{
    _namechangerDetector.OnSettingsChanged(player);
}

VoltMod::RawConVar& AntiCheatManager::CheatsConVar() const
{
    if (!_svCheats)
        _svCheats = _rt.ConVars.Raw("sv_cheats");
    return *_svCheats;
}

bool AntiCheatManager::DetectionsEnabled() const
{
    const auto& settings = _config.Get().anticheat;
    if (!settings.enabled)
        return false;
    const VoltMod::RawConVar& cheats = CheatsConVar();
    if (!cheats.Valid())
        return true;
    return !cheats.GetBool() || settings.allowSvCheatsTesting;
}

bool AntiCheatManager::EnforceCheatCvars() const
{
    const VoltMod::RawConVar& cheats = CheatsConVar();
    return ShouldEnforceCheatCvars(cheats.Valid() && cheats.GetBool(), TimeUtils::MonotonicSeconds(), _cheatGraceUntil);
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
    const VoltMod::Players::Player* player = _rt.Players.GetPlayerBySlot(slot);
    if (!player || player->IsBot())
        return false;
    VoltMod::PlayerController controller(slot);
    return controller.IsValid() && !(controller.GetFlags() & VoltMod::Sdk::FL_FAKECLIENT);
}

void AntiCheatManager::Report(int slot, const std::optional<Finding>& finding)
{
    if (finding)
        _response.Handle(slot, *finding);
}

}  // namespace Anticheat
