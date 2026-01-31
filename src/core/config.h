#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "../database/database.h"

namespace core {

using json = nlohmann::json;

/**
 * @brief Main plugin configuration
 */
struct PluginConfig
{
    std::string plugin_name = "Admin System";
    std::string version = "1.0.0";
    bool debug_mode = false;
    int ban_check_interval = 60; // seconds
    int max_warnings = 3;
    std::string default_ban_reason = "Banned by admin";
    std::string default_kick_reason = "Kicked by admin";

    /**
     * @brief Load from JSON
     * @param j JSON object
     */
    void from_json(const json& j);

    /**
     * @brief Convert to JSON
     * @return JSON object
     */
    json to_json() const;
};

/**
 * @brief Configuration manager for loading JSON config files
 */
class ConfigManager
{
public:
    /**
     * @brief Load configuration from file
     * @param file_path Path to JSON config file
     * @return true if successful
     */
    bool LoadConfig(const std::string& file_path);

    /**
     * @brief Load database configuration
     * @param file_path Path to database.json
     * @return true if successful
     */
    bool LoadDatabaseConfig(const std::string& file_path);

    /**
     * @brief Load admins from file
     * @param file_path Path to admins.json
     * @return true if successful
     */
    bool LoadAdmins(const std::string& file_path);

    /**
     * @brief Load admin groups from file
     * @param file_path Path to groups.json
     * @return true if successful
     */
    bool LoadGroups(const std::string& file_path);

    /**
     * @brief Get plugin configuration
     * @return Reference to plugin config
     */
    const PluginConfig& GetPluginConfig() const { return m_pluginConfig; }

    /**
     * @brief Get database configuration
     * @return Reference to database config
     */
    const database::DatabaseConfig& GetDatabaseConfig() const { return m_databaseConfig; }

    /**
     * @brief Get singleton instance
     * @return ConfigManager instance
     */
    static ConfigManager& Instance();

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    bool LoadJsonFile(const std::string& file_path, json& out_json);

    PluginConfig m_pluginConfig;
    database::DatabaseConfig m_databaseConfig;
};

} // namespace core
