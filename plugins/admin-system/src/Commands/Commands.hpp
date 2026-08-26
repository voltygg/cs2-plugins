#pragma once

#include "../Core/App.hpp"

#include <VoltMod/Commands/CommandManager.hpp>
#include <VoltMod/Core/Subscription.hpp>
#include <vector>

namespace AdminSystem
{

/**
 * Each source file in this directory registers one group of commands. App::RegisterCommands
 * calls them in order; the list there is the whole command surface.
 *
 * Every registration is a `Subscription` appended to @p subs, which App owns and drops last, so
 * the handlers stop before the managers they captured go away.
 */
namespace Commands
{
using Subs = std::vector<VoltMod::Subscription>;

void RegisterAdminMenuCommand(VoltMod::CommandManager& commands, App& app, Subs& subs);
void RegisterAdminSelfCommands(VoltMod::CommandManager& commands, App& app, Subs& subs);
void RegisterCheatCheckCommands(VoltMod::CommandManager& commands, App& app, Subs& subs);
void RegisterFreezeCommands(VoltMod::CommandManager& commands, App& app, Subs& subs);
void RegisterInfoCommands(VoltMod::CommandManager& commands, App& app, Subs& subs);
void RegisterPunishmentCommands(VoltMod::CommandManager& commands, App& app, Subs& subs);
void RegisterReportCommand(VoltMod::CommandManager& commands, App& app, Subs& subs);
}  // namespace Commands

}  // namespace AdminSystem
