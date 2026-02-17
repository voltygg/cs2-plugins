#pragma once

#include <functional>
#include <string>
#include <vector>

namespace AdminSystem::Players
{
class Player;
}

namespace AdminSystem::Commands
{

/** Result returned by a command handler to indicate success/failure and an optional message. */
struct CommandResult
{
    bool Success = true;
    std::string Message;
};

/** Signature for command handler callbacks: (caller, arguments) -> result. */
using CommandHandler = std::function<CommandResult(Players::Player*, const std::vector<std::string>&)>;

/** A registered chat command with metadata, permission requirements, and handler callback. */
struct Command
{
    std::string Name;
    std::vector<std::string> Aliases;
    std::string Description;
    std::string Usage;
    std::string Permission;
    int MinArgs = 0;
    int MaxArgs = 99;
    CommandHandler Handler;

    bool Matches(const std::string& cmd) const;
};

/**
 * Fluent builder for constructing Command instances.
 * Usage: CommandBuilder("kick").WithAliases({"k"}).RequirePermission("c").OnExecute(...).Build()
 */
class CommandBuilder
{
public:
    explicit CommandBuilder(const std::string& name) { _command.Name = name; }

    CommandBuilder& WithAliases(std::initializer_list<std::string> aliases)
    {
        _command.Aliases = aliases;
        return *this;
    }

    CommandBuilder& WithDescription(const std::string& desc)
    {
        _command.Description = desc;
        return *this;
    }

    CommandBuilder& WithUsage(const std::string& usage)
    {
        _command.Usage = usage;
        return *this;
    }

    CommandBuilder& RequirePermission(const std::string& flags)
    {
        _command.Permission = flags;
        return *this;
    }

    CommandBuilder& WithArgs(int min, int max = 99)
    {
        _command.MinArgs = min;
        _command.MaxArgs = max;
        return *this;
    }

    CommandBuilder& OnExecute(CommandHandler handler)
    {
        _command.Handler = std::move(handler);
        return *this;
    }

    Command Build() { return std::move(_command); }

private:
    Command _command;
};

}  // namespace AdminSystem::Commands
