#include "config.h"

#include <fstream>
#include <iostream>

namespace core
{

void PluginConfig::from_json(const json& j)
{
    if (j.contains("plugin_name"))
        plugin_name = j["plugin_name"];

    if (j.contains("version"))
        version = j["version"];

    if (j.contains("debug_mode"))
        debug_mode = j["debug_mode"];

    if (j.contains("ban_check_interval"))
        ban_check_interval = j["ban_check_interval"];

    if (j.contains("max_warnings"))
        max_warnings = j["max_warnings"];

    if (j.contains("default_ban_reason"))
        default_ban_reason = j["default_ban_reason"];

    if (j.contains("default_kick_reason"))
        default_kick_reason = j["default_kick_reason"];
}

json PluginConfig::to_json() const
{
    json j;
    j["plugin_name"] = plugin_name;
    j["version"] = version;
    j["debug_mode"] = debug_mode;
    j["ban_check_interval"] = ban_check_interval;
    j["max_warnings"] = max_warnings;
    j["default_ban_reason"] = default_ban_reason;
    j["default_kick_reason"] = default_kick_reason;
    return j;
}

bool ConfigManager::LoadConfig(const std::string& file_path)
{
    json j;
    if (!LoadJsonFile(file_path, j))
    {
        return false;
    }

    m_pluginConfig.from_json(j);
    return true;
}

bool ConfigManager::LoadDatabaseConfig(const std::string& file_path)
{
    json j;
    if (!LoadJsonFile(file_path, j))
    {
        return false;
    }

    try
    {
        if (j.contains("host"))
            m_databaseConfig.host = j["host"];

        if (j.contains("port"))
            m_databaseConfig.port = j["port"];

        if (j.contains("database"))
            m_databaseConfig.database = j["database"];

        if (j.contains("username"))
            m_databaseConfig.username = j["username"];

        if (j.contains("password"))
            m_databaseConfig.password = j["password"];

        if (j.contains("schema"))
            m_databaseConfig.schema = j["schema"];

        if (j.contains("pool_size"))
            m_databaseConfig.pool_size = j["pool_size"];

        if (j.contains("ssl_mode"))
            m_databaseConfig.ssl_mode = j["ssl_mode"];

        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Config] Error parsing database config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::LoadAdmins(const std::string& file_path)
{
    json j;
    if (!LoadJsonFile(file_path, j))
    {
        return false;
    }

    // TODO: Parse admins array and load into AdminManager
    // This will be implemented when AdminManager is ready

    return true;
}

bool ConfigManager::LoadGroups(const std::string& file_path)
{
    json j;
    if (!LoadJsonFile(file_path, j))
    {
        return false;
    }

    // TODO: Parse groups array and load into AdminManager
    // This will be implemented when AdminManager is ready

    return true;
}

bool ConfigManager::LoadJsonFile(const std::string& file_path, json& out_json)
{
    try
    {
        std::ifstream file(file_path);
        if (!file.is_open())
        {
            std::cerr << "[Config] Failed to open: " << file_path << std::endl;
            return false;
        }

        file >> out_json;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Config] Error loading " << file_path << ": " << e.what() << std::endl;
        return false;
    }
}

ConfigManager& ConfigManager::Instance()
{
    static ConfigManager instance;
    return instance;
}

}  // namespace core
