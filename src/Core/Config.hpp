#pragma once

#include "../Database/Database.hpp"
#include "Singleton.hpp"

#include <nlohmann/json_fwd.hpp>
#include <string>

namespace AdminSystem::Core
{

/** General plugin settings loaded from the "plugin" and "punishments" sections of settings.json. */
struct PluginConfig
{
    bool DebugMode = false;
    int MaxWarnings = 3;
    std::string DefaultBanReason = "Banned by administrator";
};

/**
 * Loads and owns all JSON configuration (settings.json, admins.json).
 * Provides read-only access to parsed config structs.
 */
class ConfigManager : public Singleton<ConfigManager>
{
    friend class Singleton<ConfigManager>;

public:
    /** Load the consolidated settings.json (plugin, database, commands, punishments, admin). */
    bool LoadSettings(const std::string& path);

    /** Load admins.json (groups + admins merged). */
    bool LoadAdminsConfig(const std::string& path);

    const PluginConfig& GetPluginConfig() const { return _pluginConfig; }
    const Database::DatabaseConfig& GetDatabaseConfig() const { return _databaseConfig; }

private:
    ConfigManager() = default;

    bool LoadJsonFile(const std::string& filePath, nlohmann::json& outJson);

    PluginConfig _pluginConfig;
    Database::DatabaseConfig _databaseConfig;
};

}  // namespace AdminSystem::Core
