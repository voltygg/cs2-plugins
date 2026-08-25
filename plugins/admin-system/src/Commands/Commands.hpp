#pragma once

namespace VoltMod::Commands
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
void RegisterAdminMenuCommand(VoltMod::Commands::CommandManager& commands, App& app);
void RegisterAdminSelfCommands(VoltMod::Commands::CommandManager& commands, App& app);
void RegisterCheatCheckCommands(VoltMod::Commands::CommandManager& commands, App& app);
void RegisterFreezeCommands(VoltMod::Commands::CommandManager& commands, App& app);
void RegisterInfoCommands(VoltMod::Commands::CommandManager& commands, App& app);
void RegisterMapCommands(VoltMod::Commands::CommandManager& commands, App& app);
void RegisterPunishmentCommands(VoltMod::Commands::CommandManager& commands, App& app);
void RegisterReportCommand(VoltMod::Commands::CommandManager& commands, App& app);
}  // namespace Commands

}  // namespace AdminSystem
