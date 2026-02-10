#include "command_manager.h"

#include "../admin/admin_manager.h"
#include "../player/player_manager.h"
#include "../utils/string.h"

namespace commands
{

void CommandManager::RegisterCommand(const Command& cmd)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_commands[utils::String::ToLower(cmd.name)] = cmd;
}

void CommandManager::UnregisterCommand(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_commands.erase(utils::String::ToLower(name));
}

bool CommandManager::HandleChatMessage(player::Player* player, const std::string& message)
{
    if (!player || message.empty())
        return false;

    // Check for command prefix (! or .)
    if (message[0] != '!' && message[0] != '.')
        return false;

    // Parse command and arguments
    auto parts = ParseArguments(message.substr(1));  // Skip prefix
    if (parts.empty())
        return false;

    const std::string& cmdName = parts[0];
    std::vector<std::string> args(parts.begin() + 1, parts.end());

    // Find command
    const Command* cmd = GetCommand(cmdName);
    if (!cmd)
        return false;  // Command not found

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
        auto& adminMgr = admin::AdminManager::Instance();
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

const Command* CommandManager::GetCommand(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string lowerName = utils::String::ToLower(name);

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

std::vector<const Command*> CommandManager::GetAllCommands() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<const Command*> commands;
    commands.reserve(m_commands.size());

    for (const auto& [name, cmd] : m_commands)
    {
        commands.push_back(&cmd);
    }

    return commands;
}

void CommandManager::InitializeBuiltinCommands()
{
    // Builtin commands are registered in plugin.cpp
}

std::vector<std::string> CommandManager::ParseArguments(const std::string& text) const
{
    return utils::String::Split(text, ' ');
}

CommandManager& CommandManager::Instance()
{
    static CommandManager instance;
    return instance;
}

}  // namespace commands
