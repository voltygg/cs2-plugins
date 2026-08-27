#pragma once

#include "../Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Commands/CommandManager.hpp>
#include <string>

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
/** The reason an action carries when the caller typed none. Resolved in the server language,
 *  because it lands in the database and in the broadcast, not on one player's screen. */
inline std::string ReasonOr(const VoltMod::Caller& c, const VoltMod::Args::Opt<VoltMod::Args::Rest>& typed,
                            const char* fallbackKey)
{
    return typed.Value ? typed.Value->Value : c.Tr.Get(fallbackKey);
}

void RegisterAdminMenuCommand(VoltMod::CommandManager& commands, App& app);
void RegisterAdminSelfCommands(VoltMod::CommandManager& commands, App& app);
void RegisterCheatCheckCommands(VoltMod::CommandManager& commands, App& app);
void RegisterFreezeCommands(VoltMod::CommandManager& commands, App& app);
void RegisterInfoCommands(VoltMod::CommandManager& commands, App& app);
void RegisterPunishmentCommands(VoltMod::CommandManager& commands, App& app);
void RegisterReportCommand(VoltMod::CommandManager& commands, App& app);
}  // namespace Commands

}  // namespace AdminSystem
