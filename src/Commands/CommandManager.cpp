#include "CommandManager.hpp"

#include "../Admin/AdminManager.hpp"
#include "../Players/PlayerManager.hpp"
#include "../Utils/StringUtils.hpp"

namespace AdminSystem::Commands
{

using namespace AdminSystem::Admin;
using namespace AdminSystem::Utils;

void CommandManager::Register(Command cmd)
{
    _commands[StringUtils::ToLower(cmd.Name)] = std::move(cmd);
}

void CommandManager::Unregister(const std::string& name)
{
    _commands.erase(StringUtils::ToLower(name));
}

bool CommandManager::HandleChatMessage(Players::Player* player, const std::string& message)
{
    if (!player || message.empty())
        return false;

    // Check for any configured command prefix
    bool hasPrefix = false;
    size_t prefixLen = 0;
    for (const auto& prefix : _prefixes)
    {
        if (message.size() >= prefix.size() && message.compare(0, prefix.size(), prefix) == 0)
        {
            hasPrefix = true;
            prefixLen = prefix.size();
            break;
        }
    }

    if (!hasPrefix)
        return false;

    // Parse command and arguments (skip prefix)
    auto parts = ParseArguments(message.substr(prefixLen));
    if (parts.empty())
        return false;

    const std::string& cmdName = parts[0];
    std::vector<std::string> args(parts.begin() + 1, parts.end());

    // Find command
    const Command* cmd = GetCommand(cmdName);
    if (!cmd)
        return false;

    // Check argument count
    if (static_cast<int>(args.size()) < cmd->MinArgs)
    {
        // TODO: Send usage message to player
        return true;
    }

    if (cmd->MaxArgs != 99 && static_cast<int>(args.size()) > cmd->MaxArgs)
    {
        // TODO: Send "too many arguments" message
        return true;
    }

    // Check permissions
    if (!cmd->Permission.empty())
    {
        auto& adminMgr = AdminManager::Instance();
        if (!adminMgr.HasAnyPermission(player->GetSteamID(), cmd->Permission))
        {
            // TODO: Send "no permission" message
            return true;
        }
    }

    // Execute command
    if (cmd->Handler)
    {
        auto result = cmd->Handler(player, args);
        // TODO: Send result message to player
    }

    return true;
}

const Command* CommandManager::GetCommand(const std::string& name) const
{
    std::string lowerName = StringUtils::ToLower(name);

    // Direct lookup
    auto it = _commands.find(lowerName);
    if (it != _commands.end())
        return &it->second;

    // Check aliases
    for (const auto& [key, cmd] : _commands)
    {
        if (cmd.Matches(name))
            return &cmd;
    }

    return nullptr;
}

std::vector<const Command*> CommandManager::GetAllCommands() const
{
    std::vector<const Command*> commands;
    commands.reserve(_commands.size());

    for (const auto& [name, cmd] : _commands)
    {
        commands.push_back(&cmd);
    }

    return commands;
}

std::vector<std::string> CommandManager::ParseArguments(const std::string& text) const
{
    return StringUtils::Split(text, ' ');
}

}  // namespace AdminSystem::Commands
