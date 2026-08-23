#include "App.hpp"

#include <CS2Kit/Api.hpp>

namespace Anticheat
{

bool App::Start()
{
    if (!CS2Kit::LoadStandardConfig(Runtime, Config, {.Addon = AddonName, .Translations = false}))
        return false;

    // A missing data file leaves the two table-driven modules inert rather than taking the plugin
    // down: the aim modules, which carry no data file, are the ones worth keeping alive.
    Runtime.LoadReport.Run("Detection data", [this] {
        if (!Detections.Load(DetectionDataPath))
            return CS2Kit::StageResult::Degraded("file unreadable; DLL injection and invalid cvar modules are inert");
        return CS2Kit::StageResult::Ok(DetectionDataPath);
    });

    Response.Initialize();
    AntiCheat.Initialize();

    CS2Kit::Log::Info("Mode: {}.", Config.Get().anticheat.mode);
    return true;
}

}  // namespace Anticheat
