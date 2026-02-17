#pragma once

#include "../Database/Database.hpp"

#include <CS2Kit/Core/Singleton.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>

namespace AdminSystem::Core
{

using namespace CS2Kit::Core;

using json = nlohmann::json;
using DatabaseConfig = AdminSystem::Database::DatabaseConfig;

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
public:
    explicit ConfigManager(Token) {}

    /** Load the consolidated settings.json (plugin, database, commands, punishments, admin). */
    bool LoadSettings(const std::string& path);

    /** Load admins.json (groups + admins merged). */
    bool LoadAdminsConfig(const std::string& path);

    const PluginConfig& GetPluginConfig() const { return _pluginConfig; }
    const DatabaseConfig& GetDatabaseConfig() const { return _databaseConfig; }

private:
    bool LoadJsonFile(const std::string& filePath, json& outJson);

    PluginConfig _pluginConfig;
    DatabaseConfig _databaseConfig;
};

}  // namespace AdminSystem::Core
