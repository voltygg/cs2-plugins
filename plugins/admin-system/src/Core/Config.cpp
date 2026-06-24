#include "Config.hpp"

#include <CS2Kit/Utils/Json.hpp>
#include <CS2Kit/Utils/Log.hpp>

namespace AdminSystem::Core
{

using namespace CS2Kit::Utils;

bool ConfigManager::LoadSettings(const std::string& path)
{
    auto loaded = Json::TryDeserializeFile<Settings>(path);
    if (!loaded)
        return false;

    _settings = std::move(*loaded);
    Log::Info("Loaded settings from {}", path);
    return true;
}

}  // namespace AdminSystem::Core
