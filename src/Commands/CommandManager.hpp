#pragma once

#include "../Core/Singleton.hpp"
#include "Command.hpp"

#include <unordered_map>
#include <vector>

namespace AdminSystem::Commands
{

/**
 * Dispatches chat commands (prefixed with ! or .) to registered handlers.
 * Handles prefix matching, argument parsing, and permission enforcement.
 */
class CommandManager : public Core::Singleton<CommandManager>
{
public:
    explicit CommandManager(Token) { _prefixes = {"!", "."}; }

    void Register(Command cmd);
    void Unregister(const std::string& name);
    bool HandleChatMessage(Players::Player* player, const std::string& message);
    const Command* GetCommand(const std::string& name) const;
    std::vector<const Command*> GetAllCommands() const;

    void SetPrefixes(const std::vector<std::string>& prefixes) { _prefixes = prefixes; }

private:
    std::vector<std::string> ParseArguments(const std::string& text) const;

    std::unordered_map<std::string, Command> _commands;
    std::vector<std::string> _prefixes;
};

}  // namespace AdminSystem::Commands
