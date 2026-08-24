#include "App.hpp"

#include <VoltMod/Api.hpp>

namespace Anticheat
{

bool App::Start()
{
    if (!VoltMod::LoadStandardConfig(Runtime, Config, {.Addon = AddonName, .Translations = false}))
        return false;

    // A missing data file leaves the two table-driven modules inert rather than taking the plugin
    // down: the aim modules, which carry no data file, are the ones worth keeping alive.
    Runtime.LoadReport.Run("Detection data", [this] {
        if (!Detections.Load(DetectionDataPath))
            return VoltMod::StageResult::Degraded("file unreadable; DLL injection and invalid cvar modules are inert");
        return VoltMod::StageResult::Ok(DetectionDataPath);
    });

    Response.Initialize();
    AntiCheat.Initialize();

    VoltMod::Log::Info("Mode: {}.", Config.Get().anticheat.mode);
    return true;
}

}  // namespace Anticheat
