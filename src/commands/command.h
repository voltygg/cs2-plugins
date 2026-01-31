#pragma once

#include <string>
#include <vector>
#include <functional>

// Forward declarations
namespace player {
class Player;
}

namespace commands {

/**
 * @brief Command execution result
 */
struct CommandResult
{
    bool success{true};
    std::string message;
};

/**
 * @brief Command handler function signature
 */
using CommandHandler = std::function<CommandResult(player::Player*, const std::vector<std::string>&)>;

/**
 * @brief Command definition
 */
struct Command
{
    std::string name;                    // Primary command name
    std::vector<std::string> aliases;    // Alternative names
    std::string description;             // Help text
    std::string usage;                   // Usage syntax
    std::string permission;              // Required permission flag(s)
    int min_args{0};                     // Minimum arguments
    int max_args{99};                    // Maximum arguments (-1 = unlimited)
    CommandHandler handler;              // Execution function

    /**
     * @brief Check if command matches name or alias
     */
    bool Matches(const std::string& cmd) const;
};

} // namespace commands
