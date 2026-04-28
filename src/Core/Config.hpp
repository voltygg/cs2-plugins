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

/** Chat formatting settings loaded from the "chat" section of settings.json. */
struct ChatConfig
{
    /** If true, every issued punishment broadcasts a colored line to all players. */
    bool BroadcastPunishments = true;

    /** If true, an admin's chat is intercepted and re-emitted with their group's colored prefix. */
    bool TagAdminChatMessages = true;

    /** Prefix used when an admin doesn't belong to any group with `ChatPrefix` set. */
    std::string FallbackPrefix = "[ADMIN]";
    std::string FallbackPrefixColor = "red";
    std::string FallbackNameColor = "default";
    std::string FallbackMessageColor = "default";
};

/**
 * Loads and owns settings.json. All admin/group data is owned by the database
 * (`admins` and `admin_groups` tables) — this manager only exposes plugin/DB/chat config.
 */
class ConfigManager : public Singleton<ConfigManager>
{
public:
    explicit ConfigManager(Token) {}

    /** Load settings.json (plugin, database, punishments, chat sections). */
    bool LoadSettings(const std::string& path);

    const PluginConfig& GetPluginConfig() const { return _pluginConfig; }
    const DatabaseConfig& GetDatabaseConfig() const { return _databaseConfig; }
    const ChatConfig& GetChatConfig() const { return _chatConfig; }

private:
    bool LoadJsonFile(const std::string& filePath, json& outJson);

    PluginConfig _pluginConfig;
    DatabaseConfig _databaseConfig;
    ChatConfig _chatConfig;
};

}  // namespace AdminSystem::Core
