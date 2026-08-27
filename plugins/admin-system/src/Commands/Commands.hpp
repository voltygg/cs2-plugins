#pragma once

#include "../Core/App.hpp"

#include <VoltMod/Commands/CommandManager.hpp>

namespace AdminSystem
{

/**
 * Each source file in this directory registers one group of commands. App::RegisterCommands
 * calls them in order; the list there is the whole command surface.
 *
 * The manager owns the registrations and drops them before App does, so a handler cannot
 * outlive the managers it captured.
 */
namespace Commands
{
void RegisterAdminMenuCommand(VoltMod::CommandManager& commands, App& app);
void RegisterAdminSelfCommands(VoltMod::CommandManager& commands, App& app);
void RegisterCheatCheckCommands(VoltMod::CommandManager& commands, App& app);
void RegisterFreezeCommands(VoltMod::CommandManager& commands, App& app);
void RegisterInfoCommands(VoltMod::CommandManager& commands, App& app);
void RegisterPunishmentCommands(VoltMod::CommandManager& commands, App& app);
void RegisterReportCommand(VoltMod::CommandManager& commands, App& app);
}  // namespace Commands

}  // namespace AdminSystem
