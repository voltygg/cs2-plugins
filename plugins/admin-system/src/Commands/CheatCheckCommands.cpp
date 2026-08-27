#include "../Admin/Actions/Descriptors.hpp"
#include "../Admin/CheatCheck/CheatCheckManager.hpp"
#include "../Core/App.hpp"
#include "../Core/Permissions.hpp"
#include "Commands.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Runtime.hpp>

using VoltMod::Caller;
using VoltMod::Reply;
using VoltMod::Result;

namespace Args = VoltMod::Args;

namespace AdminSystem::Commands
{

using AdminSystem::Admin::CheatCheck::CheatCheckManager;

void RegisterCheatCheckCommands(VoltMod::CommandManager& commands, App& app, Subs& subs)
{
    subs.push_back(commands.Add("cc")
                       .Describe("Submit your verification link for a pending cheat check.")
                       .Run([&app](Caller c, Args::Word link) -> Result<Reply> {
                           switch (app.CheatCheck.SubmitPlayerLink(c.Slot, link.Value))
                           {
                           case CheatCheckManager::SubmitResult::Relayed:
                               return c.Ok("cheatCheck.linkReceived");
                           case CheatCheckManager::SubmitResult::Invalid:
                               return c.Fail("cheatCheck.linkInvalid");
                           case CheatCheckManager::SubmitResult::NoActiveCheck:
                           default:
                               return c.Fail("cheatCheck.noActiveCheck");
                           }
                       }));

    subs.push_back(commands.Add("check")
                       .Describe("Start a cheat check on a player.")
                       .Permission(Flag(Permission::Control))
                       .Run([&app](Caller c, Args::Target t) -> Result<Reply> {
                           if (!AdminSystem::Admin::Actions::CallCheck(app, app.Runtime.Players.RefFor(c.Slot), t.Value->Ref()))
                               return c.Fail("cmd.noPermission");
                           return c.Ok("cheatCheck.started", {{"name", t.Value->Name()}});
                       }));

    subs.push_back(commands.Add("cccancel")
                       .Alias("uncheck")
                       .Describe("Cancel a pending cheat check on a player.")
                       .Permission(Flag(Permission::Control))
                       .Run([&app](Caller c, Args::Target t) -> Result<Reply> {
                           if (!AdminSystem::Admin::Actions::CancelCheck(app, app.Runtime.Players.RefFor(c.Slot), t.Value->Ref()))
                               return c.Fail("cheatCheck.noActiveCheck");
                           return c.Ok("cheatCheck.cancelled", {{"name", t.Value->Name()}});
                       }));
}

}  // namespace AdminSystem::Commands
