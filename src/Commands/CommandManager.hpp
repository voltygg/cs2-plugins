#pragma once

#include "Command.hpp"
#include "../Core/Singleton.hpp"
#include <unordered_map>
#include <vector>

namespace AdminSystem::Commands {

class CommandManager : public Core::Singleton<CommandManager> {
    friend class Core::Singleton<CommandManager>;

public:
    void Register(Command cmd);
    void Unregister(const std::string& name);
    bool HandleChatMessage(Players::Player* player, const std::string& message);
    const Command* GetCommand(const std::string& name) const;
    std::vector<const Command*> GetAllCommands() const;

    void SetPrefixes(const std::vector<std::string>& prefixes) { _prefixes = prefixes; }

private:
    CommandManager() { _prefixes = {"!", "."}; }

    std::vector<std::string> ParseArguments(const std::string& text) const;

    std::unordered_map<std::string, Command> _commands;
    std::vector<std::string> _prefixes;
};

} // namespace AdminSystem::Commands
