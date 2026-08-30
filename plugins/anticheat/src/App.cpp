#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <format>

namespace Anticheat
{

bool App::Start()
{
    if (!VoltMod::LoadStandardConfig(Runtime, Config, {.Addon = AddonName, .Translations = false}))
        return false;

    // A missing data file leaves the two table-driven modules inert rather than taking the plugin
    // down: the aim modules, which carry no data file, are the ones worth keeping alive.
    Runtime.LoadReport.Run("Detection data", [this] {
        if (auto loaded = Detections.Load(DetectionDataPath); !loaded)
            return VoltMod::StageResult::Degraded(
                std::format("{}; DLL injection and invalid cvar modules are inert", loaded.error().Detail));
        return VoltMod::StageResult::Ok(DetectionDataPath);
    });

    Response.Initialize();
    AntiCheat.Initialize();

    VoltMod::Log::Info("Mode: {}.", Config.Get().anticheat.mode);
    return true;
}

}  // namespace Anticheat
