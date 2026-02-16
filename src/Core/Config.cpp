#include "Config.hpp"
#include "../Admin/AdminManager.hpp"
#include "../Utils/Log.hpp"

#include <ISmmPlugin.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

using json = nlohmann::json;
using namespace AdminSystem::Admin;
using namespace AdminSystem::Utils;

namespace AdminSystem::Core {

bool ConfigManager::LoadSettings(const std::string& path)
{
    json j;
    if (!LoadJsonFile(path, j))
        return false;

    // Plugin section
    if (j.contains("plugin"))
    {
        auto& p = j["plugin"];
        if (p.contains("logLevel"))
            _pluginConfig.DebugMode = (p["logLevel"] == "debug");
    }

    // Database section
    if (j.contains("database"))
    {
        auto& db = j["database"];
        if (db.contains("host"))       _databaseConfig.Host = db["host"];
        if (db.contains("port"))       _databaseConfig.Port = db["port"];
        if (db.contains("database"))   _databaseConfig.DatabaseName = db["database"];
        if (db.contains("username"))   _databaseConfig.Username = db["username"];
        if (db.contains("password"))   _databaseConfig.Password = db["password"];
        if (db.contains("schema"))     _databaseConfig.Schema = db["schema"];
        if (db.contains("sslMode"))    _databaseConfig.SslMode = db["sslMode"];
    }

    // Punishments section
    if (j.contains("punishments"))
    {
        auto& pun = j["punishments"];
        if (pun.contains("defaultBanReason"))
            _pluginConfig.DefaultBanReason = pun["defaultBanReason"];
        if (pun.contains("warningThreshold"))
            _pluginConfig.MaxWarnings = pun["warningThreshold"];
    }

    Log::Info("Loaded settings from {}", path);
    return true;
}

bool ConfigManager::LoadAdminsConfig(const std::string& path)
{
    json j;
    if (!LoadJsonFile(path, j))
        return false;

    auto& adminMgr = AdminManager::Instance();

    // Load groups
    if (j.contains("groups") && j["groups"].is_array())
    {
        int groupCount = 0;
        for (const auto& entry : j["groups"])
        {
            try
            {
                Database::AdminGroup group;
                if (entry.contains("name"))      group.Name = entry["name"];
                if (entry.contains("flags"))     group.Flags = entry["flags"];
                if (entry.contains("immunity"))  group.Immunity = entry["immunity"];

                if (entry.contains("inherits") && entry["inherits"].is_array())
                {
                    for (const auto& parent : entry["inherits"])
                        group.Inherits.push_back(parent.get<std::string>());
                }

                adminMgr.AddGroup(group);
                ++groupCount;
            }
            catch (const std::exception& e)
            {
                Log::Warn("Failed to parse group entry: {}", e.what());
            }
        }
        Log::Info("Loaded {} group(s) from config.", groupCount);
    }

    // Load admins
    if (j.contains("admins") && j["admins"].is_array())
    {
        int adminCount = 0;
        for (const auto& entry : j["admins"])
        {
            try
            {
                Database::Admin admin;
                if (entry.contains("steamId"))
                    admin.SteamId = std::stoll(entry["steamId"].get<std::string>());
                if (entry.contains("name"))
                    admin.Name = entry["name"].get<std::string>();
                if (entry.contains("flags"))
                    admin.Flags = entry["flags"].get<std::string>();
                if (entry.contains("immunity"))
                    admin.Immunity = entry["immunity"].get<int32_t>();

                if (entry.contains("groups") && entry["groups"].is_array())
                {
                    for (const auto& group : entry["groups"])
                        admin.Groups.push_back(group.get<std::string>());
                }

                adminMgr.AddAdmin(admin);
                ++adminCount;
            }
            catch (const std::exception& e)
            {
                Log::Warn("Failed to parse admin entry: {}", e.what());
            }
        }
        Log::Info("Loaded {} admin(s) from config.", adminCount);
    }

    return true;
}

bool ConfigManager::LoadJsonFile(const std::string& filePath, json& outJson)
{
    try
    {
        std::filesystem::path resolvedPath(filePath);
        if (resolvedPath.is_relative() && g_SMAPI)
        {
            resolvedPath = std::filesystem::path(g_SMAPI->GetBaseDir()) / filePath;
        }

        std::ifstream file(resolvedPath);
        if (!file.is_open())
        {
            Log::Error("Failed to open: {}", resolvedPath.string());
            return false;
        }

        file >> outJson;
        return true;
    }
    catch (const std::exception& e)
    {
        Log::Error("Error loading {}: {}", filePath, e.what());
        return false;
    }
}

} // namespace AdminSystem::Core
