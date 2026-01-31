#pragma once

#include "command.h"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace commands {

/**
 * @brief Command manager for registration and dispatch
 */
class CommandManager
{
public:
    /**
     * @brief Register a command
     */
    void RegisterCommand(const Command& cmd);

    /**
     * @brief Unregister a command by name
     */
    void UnregisterCommand(const std::string& name);

    /**
     * @brief Parse and execute a chat message
     * @param player Player who sent the message
     * @param message Chat message text
     * @return true if command was handled
     */
    bool HandleChatMessage(player::Player* player, const std::string& message);

    /**
     * @brief Get command by name or alias
     */
    const Command* GetCommand(const std::string& name) const;

    /**
     * @brief Get all registered commands
     */
    std::vector<const Command*> GetAllCommands() const;

    /**
     * @brief Initialize built-in commands
     */
    void InitializeBuiltinCommands();

    /**
     * @brief Get singleton instance
     */
    static CommandManager& Instance();

private:
    CommandManager() = default;
    ~CommandManager() = default;
    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;

    /**
     * @brief Parse command arguments
     */
    std::vector<std::string> ParseArguments(const std::string& text) const;

    std::unordered_map<std::string, Command> m_commands;
    mutable std::mutex m_mutex;
};

} // namespace commands
