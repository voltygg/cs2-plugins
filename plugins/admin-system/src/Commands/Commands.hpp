#pragma once

namespace CS2Kit::Commands
{
class CommandManager;
}

namespace AdminSystem
{
struct App;

/**
 * Each source file in this directory registers one group of commands. App::RegisterCommands
 * calls them in order; the list there is the whole command surface.
 */
namespace Commands
{
void RegisterAdminMenuCommand(CS2Kit::Commands::CommandManager& commands, App& app);
void RegisterAdminSelfCommands(CS2Kit::Commands::CommandManager& commands, App& app);
void RegisterCheatCheckCommands(CS2Kit::Commands::CommandManager& commands, App& app);
void RegisterFreezeCommands(CS2Kit::Commands::CommandManager& commands, App& app);
void RegisterInfoCommands(CS2Kit::Commands::CommandManager& commands, App& app);
void RegisterPunishmentCommands(CS2Kit::Commands::CommandManager& commands, App& app);
void RegisterReportCommand(CS2Kit::Commands::CommandManager& commands, App& app);
}  // namespace Commands

}  // namespace AdminSystem
