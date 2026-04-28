#include "Config.hpp"

#include <CS2Kit/Core/Paths.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace AdminSystem::Core
{

using namespace CS2Kit::Core;
using namespace CS2Kit::Utils;

using json = nlohmann::json;
using DatabaseConfig = AdminSystem::Database::DatabaseConfig;

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
        if (p.contains("locale"))
            _pluginConfig.Locale = p["locale"];
    }

    // Database section
    if (j.contains("database"))
    {
        auto& db = j["database"];
        if (db.contains("host"))
            _databaseConfig.Host = db["host"];
        if (db.contains("port"))
            _databaseConfig.Port = db["port"];
        if (db.contains("database"))
            _databaseConfig.DatabaseName = db["database"];
        if (db.contains("username"))
            _databaseConfig.Username = db["username"];
        if (db.contains("password"))
            _databaseConfig.Password = db["password"];
        if (db.contains("schema"))
            _databaseConfig.Schema = db["schema"];
        if (db.contains("sslMode"))
            _databaseConfig.SslMode = db["sslMode"];
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

    // Chat section
    if (j.contains("chat"))
    {
        auto& chat = j["chat"];
        if (chat.contains("broadcastPunishments"))
            _chatConfig.BroadcastPunishments = chat["broadcastPunishments"];
        if (chat.contains("tagAdminChatMessages"))
            _chatConfig.TagAdminChatMessages = chat["tagAdminChatMessages"];
        if (chat.contains("fallbackPrefix"))
            _chatConfig.FallbackPrefix = chat["fallbackPrefix"];
        if (chat.contains("fallbackPrefixColor"))
            _chatConfig.FallbackPrefixColor = chat["fallbackPrefixColor"];
        if (chat.contains("fallbackNameColor"))
            _chatConfig.FallbackNameColor = chat["fallbackNameColor"];
        if (chat.contains("fallbackMessageColor"))
            _chatConfig.FallbackMessageColor = chat["fallbackMessageColor"];
    }

    Log::Info("Loaded settings from {}", path);
    return true;
}

bool ConfigManager::LoadJsonFile(const std::string& filePath, json& outJson)
{
    try
    {
        auto resolvedPath = ResolvePath(filePath);

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

}  // namespace AdminSystem::Core
