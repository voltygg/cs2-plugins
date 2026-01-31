#include "command_manager.h"
#include "../player/player_manager.h"
#include "../admin/admin_manager.h"
#include "../utils/string.h"
#include <algorithm>

using namespace std;
using namespace player;
using namespace admin;
using namespace utils;

namespace commands {

void CommandManager::RegisterCommand(const Command& cmd)
{
    lock_guard<mutex> lock(m_mutex);
    m_commands[String::ToLower(cmd.name)] = cmd;
}

void CommandManager::UnregisterCommand(const string& name)
{
    lock_guard<mutex> lock(m_mutex);
    m_commands.erase(String::ToLower(name));
}

bool CommandManager::HandleChatMessage(Player* player, const string& message)
{
    if (!player || message.empty())
        return false;

    // Check for command prefix (! or .)
    if (message[0] != '!' && message[0] != '.')
        return false;

    // Parse command and arguments
    auto parts = ParseArguments(message.substr(1)); // Skip prefix
    if (parts.empty())
        return false;

    const string& cmdName = parts[0];
    vector<string> args(parts.begin() + 1, parts.end());

    // Find command
    const Command* cmd = GetCommand(cmdName);
    if (!cmd)
        return false; // Command not found

    // Check argument count
    if (args.size() < static_cast<size_t>(cmd->min_args))
    {
        // TODO: Send usage message to player
        return true;
    }

    if (cmd->max_args != -1 && args.size() > static_cast<size_t>(cmd->max_args))
    {
        // TODO: Send "too many arguments" message
        return true;
    }

    // Check permissions
    if (!cmd->permission.empty())
    {
        auto& adminMgr = AdminManager::Instance();
        if (!adminMgr.HasAnyPermission(player->GetSteamID(), cmd->permission))
        {
            // TODO: Send "no permission" message
            return true;
        }
    }

    // Execute command
    if (cmd->handler)
    {
        auto result = cmd->handler(player, args);
        // TODO: Send result message to player
    }

    return true;
}

const Command* CommandManager::GetCommand(const string& name) const
{
    lock_guard<mutex> lock(m_mutex);

    string lowerName = String::ToLower(name);

    // Direct lookup
    auto it = m_commands.find(lowerName);
    if (it != m_commands.end())
        return &it->second;

    // Check aliases
    for (const auto& [key, cmd] : m_commands)
    {
        if (cmd.Matches(name))
            return &cmd;
    }

    return nullptr;
}

vector<const Command*> CommandManager::GetAllCommands() const
{
    lock_guard<mutex> lock(m_mutex);

    vector<const Command*> commands;
    commands.reserve(m_commands.size());

    for (const auto& [name, cmd] : m_commands)
    {
        commands.push_back(&cmd);
    }

    return commands;
}

void CommandManager::InitializeBuiltinCommands()
{
    // Kick command
    RegisterCommand({
        .name = "kick",
        .aliases = {"k"},
        .description = "Kick a player from the server",
        .usage = "!kick <player> [reason]",
        .permission = "c",
        .min_args = 1,
        .handler = [](Player* admin, const vector<string>& args) -> CommandResult {
            // TODO: Implement kick logic
            return {true, "Kick command executed"};
        }
    });

    // Ban command
    RegisterCommand({
        .name = "ban",
        .aliases = {"b"},
        .description = "Ban a player from the server",
        .usage = "!ban <player> <duration> [reason]",
        .permission = "b",
        .min_args = 2,
        .handler = [](Player* admin, const vector<string>& args) -> CommandResult {
            // TODO: Implement ban logic
            return {true, "Ban command executed"};
        }
    });

    // Mute command
    RegisterCommand({
        .name = "mute",
        .aliases = {"m"},
        .description = "Mute a player's voice",
        .usage = "!mute <player> <duration> [reason]",
        .permission = "i",
        .min_args = 2,
        .handler = [](Player* admin, const vector<string>& args) -> CommandResult {
            // TODO: Implement mute logic
            return {true, "Mute command executed"};
        }
    });

    // Gag command
    RegisterCommand({
        .name = "gag",
        .aliases = {"g"},
        .description = "Gag a player's chat",
        .usage = "!gag <player> <duration> [reason]",
        .permission = "i",
        .min_args = 2,
        .handler = [](Player* admin, const vector<string>& args) -> CommandResult {
            // TODO: Implement gag logic
            return {true, "Gag command executed"};
        }
    });

    // Warn command
    RegisterCommand({
        .name = "warn",
        .aliases = {"w"},
        .description = "Warn a player",
        .usage = "!warn <player> [reason]",
        .permission = "i",
        .min_args = 1,
        .handler = [](Player* admin, const vector<string>& args) -> CommandResult {
            // TODO: Implement warn logic
            return {true, "Warn command executed"};
        }
    });
}

vector<string> CommandManager::ParseArguments(const string& text) const
{
    return String::Split(text, ' ');
}

CommandManager& CommandManager::Instance()
{
    static CommandManager instance;
    return instance;
}

} // namespace commands
