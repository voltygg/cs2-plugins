#include "config.h"

#include "../admin/admin_manager.h"

#include <ISmmPlugin.h>
#include <filesystem>
#include <fstream>
#include <iostream>

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

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
        std::cerr << "[AdminSystem] Error parsing database config: " << e.what() << std::endl;
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

    if (!j.contains("admins") || !j["admins"].is_array())
    {
        META_CONPRINTF("[AdminSystem] admins.json missing 'admins' array\n");
        return false;
    }

    auto& admin_mgr = admin::AdminManager::Instance();
    int count = 0;

    for (const auto& entry : j["admins"])
    {
        try
        {
            database::Admin admin;

            if (entry.contains("steam_id"))
                admin.steam_id = std::stoll(entry["steam_id"].get<std::string>());

            if (entry.contains("name"))
                admin.name = entry["name"].get<std::string>();

            if (entry.contains("flags"))
                admin.flags = entry["flags"].get<std::string>();

            if (entry.contains("immunity"))
                admin.immunity = entry["immunity"].get<int32_t>();

            if (entry.contains("groups") && entry["groups"].is_array())
            {
                for (const auto& group : entry["groups"])
                    admin.groups.push_back(group.get<std::string>());
            }

            admin_mgr.AddAdmin(admin);
            ++count;
        }
        catch (const std::exception& e)
        {
            META_CONPRINTF("[AdminSystem] Warning: Failed to parse admin entry: %s\n", e.what());
        }
    }

    META_CONPRINTF("[AdminSystem] Loaded %d admin(s) from file.\n", count);
    return true;
}

bool ConfigManager::LoadGroups(const std::string& file_path)
{
    json j;
    if (!LoadJsonFile(file_path, j))
    {
        return false;
    }

    if (!j.contains("groups") || !j["groups"].is_array())
    {
        META_CONPRINTF("[AdminSystem] groups.json missing 'groups' array\n");
        return false;
    }

    auto& admin_mgr = admin::AdminManager::Instance();
    int count = 0;

    for (const auto& entry : j["groups"])
    {
        try
        {
            database::AdminGroup group;

            if (entry.contains("name"))
                group.name = entry["name"].get<std::string>();

            if (entry.contains("flags"))
                group.flags = entry["flags"].get<std::string>();

            if (entry.contains("immunity"))
                group.immunity = entry["immunity"].get<int32_t>();

            if (entry.contains("inherits") && entry["inherits"].is_array())
            {
                for (const auto& parent : entry["inherits"])
                    group.inherits.push_back(parent.get<std::string>());
            }

            admin_mgr.AddGroup(group);
            ++count;
        }
        catch (const std::exception& e)
        {
            META_CONPRINTF("[AdminSystem] Warning: Failed to parse group entry: %s\n", e.what());
        }
    }

    META_CONPRINTF("[AdminSystem] Loaded %d group(s) from file.\n", count);
    return true;
}

bool ConfigManager::LoadJsonFile(const std::string& file_path, json& out_json)
{
    try
    {
        // Resolve relative paths against Metamod's game base directory (game/csgo/)
        std::filesystem::path resolved_path(file_path);
        if (resolved_path.is_relative() && g_SMAPI)
        {
            resolved_path = std::filesystem::path(g_SMAPI->GetBaseDir()) / file_path;
        }

        std::ifstream file(resolved_path);
        if (!file.is_open())
        {
            META_CONPRINTF("[AdminSystem] Failed to open: %s\n", resolved_path.string().c_str());
            return false;
        }

        file >> out_json;
        return true;
    }
    catch (const std::exception& e)
    {
        META_CONPRINTF("[AdminSystem] Error loading %s: %s\n", file_path.c_str(), e.what());
        return false;
    }
}

ConfigManager& ConfigManager::Instance()
{
    static ConfigManager instance;
    return instance;
}

}  // namespace core
