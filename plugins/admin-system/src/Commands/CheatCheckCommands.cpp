#include "../Admin/Actions/Descriptors.hpp"
#include "../Admin/CheatCheck/CheatCheckManager.hpp"
#include "../Core/App.hpp"
#include "../Core/Permissions.hpp"
#include "Commands.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Runtime.hpp>
#include <format>

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using AdminSystem::Admin::CheatCheck::CheatCheckManager;

void RegisterCheatCheckCommands(CS2Kit::CommandManager& commands, App& app)
{
    commands.Register({
        .Name = "cc",
        .Description = "Submit your verification link for a pending cheat check.",
        .Args = {Word()},
        .Handler =
            [&app](CommandContext& c) {
                switch (app.CheatCheck.SubmitPlayerLink(c.CallerSlot(), c.Word))
                {
                case CheatCheckManager::SubmitResult::Relayed:
                    return c.Ok("cheatCheck.linkReceived");
                case CheatCheckManager::SubmitResult::Invalid:
                    return c.Fail("cheatCheck.linkInvalid");
                case CheatCheckManager::SubmitResult::NoActiveCheck:
                default:
                    return c.Fail("cheatCheck.noActiveCheck");
                }
            },
    });

    commands.Register({
        .Name = "check",
        .Description = "Start a cheat check on a player.",
        .Permission = Flag(Permission::Control),
        .Args = {Target()},
        .Handler =
            [&app](CommandContext& c) {
                if (!AdminSystem::Admin::Actions::CallCheck(app, c.CallerSlot(), c.Target().GetSlot()))
                    return c.Fail("cmd.noPermission");
                return c.Ok("cheatCheck.started", {{"name", c.Target().GetName()}});
            },
    });

    commands.Register({
        .Name = "cccancel",
        .Aliases = {"uncheck"},
        .Description = "Cancel a pending cheat check on a player.",
        .Permission = Flag(Permission::Control),
        .Args = {Target()},
        .Handler =
            [&app](CommandContext& c) {
                if (!AdminSystem::Admin::Actions::CancelCheck(app, c.CallerSlot(), c.Target().GetSlot()))
                    return c.Fail("cheatCheck.noActiveCheck");
                return c.Ok("cheatCheck.cancelled", {{"name", c.Target().GetName()}});
            },
    });
}

}  // namespace AdminSystem::Commands
