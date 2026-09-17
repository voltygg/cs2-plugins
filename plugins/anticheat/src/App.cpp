#include "App.hpp"

#include "Engine/Commands.hpp"
#include "Engine/StatusReport.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Strings.hpp>
#include <VoltMod/Core/Time.hpp>
#include <string>
#include <tuple>
#include <vector>

namespace Log = VoltMod::Log;

namespace Anticheat
{

bool App::Start()
{
    if (!VoltMod::LoadStandardConfig(Runtime, Config, {.Addon = AddonName, .Translations = false}))
        return false;

    // A missing data file leaves the two table-driven rules inert rather than taking the plugin
    // down: the aim rules, which carry no data file, are the ones worth keeping alive.
    Runtime.LoadSteps.Optional("Detection data", [this] {
        VoltMod::Status loaded = RuleTables.Load(DetectionDataPath);
        if (!loaded)
            loaded.error().Detail += "; DLL injection and invalid cvar rules are inert";
        return loaded;
    });

    Detection.Initialize();
    Simulator.Initialize();
    Dump.Initialize();
    Carried.Initialize();

    _subs.Add(Runtime.Slots.Changed += [this](int slot) { OnSlotChanged(slot); });
    _subs.Add(Runtime.Players.FullyConnected += [this](VoltMod::Player& player) { OnPlayerFullyConnected(player); });
    _subs.Add(Runtime.Players.SettingsChanged += [this](VoltMod::Player& player) { Names.OnSettingsChanged(player); });
    _subs.Add(Runtime.ConVars.Changed += [this](const VoltMod::ConVarChange& change) {
        if (Detection.OnConVarChanged(change))
            ResetInFlight();
    });

    LoadDetectionData();
    Feed.Initialize();
    Names.Initialize();
    DllScan.Initialize();
    Cvars.Initialize();

    Runtime.Status.RegisterSection("anticheat", [this] { return StatusJson(*this); });
    RegisterCommands(*this);
    Log::Info("Detection rules ready (mode={}).", Config.Get().anticheat.mode);
    return true;
}

void App::LoadDetectionData()
{
    const DetectionData& data = RuleTables.Get();
    const std::vector<std::string> rejected = Detection.InvalidCvars.LoadRules(data.cvarRules);

    if (!rejected.empty())
        Log::Warn("Ignoring duplicate cvar rule(s): {}.", VoltMod::Strings::Join(rejected, ", "));

    Log::Info("Detection data: {} cvar rule(s), {} blacklisted event(s).", Detection.InvalidCvars.Rules().Size(),
              data.dllEventBlacklist.size());
}

void App::ResetInFlight()
{
    std::apply([](auto&... modules) { (modules.Reset(), ...); }, Modules());
}

void App::ForgetEvidence()
{
    ResetInFlight();
    Detection.ForgetEvidence();
    Carried.Reset();
}

void App::OnMapStart()
{
    Detection.RefreshTeamRules();
    // In-flight tracking is keyed by ticks and positions the new map invalidates. What a player
    // has already earned is not, so it rides the map change out.
    ResetInFlight();
}

void App::OnSlotChanged(int slot)
{
    std::apply([slot](auto&... modules) { (modules.OnSlotChanged(slot), ...); }, Modules());
}

void App::OnPlayerFullyConnected(VoltMod::Player& player)
{
    // The polls seed themselves on their next tick; only the baseline has to be taken now.
    Names.OnFullyConnected(player);
}

}  // namespace Anticheat
