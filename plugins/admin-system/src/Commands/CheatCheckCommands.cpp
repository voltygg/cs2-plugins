#include "../Admin/Actions/Descriptors.hpp"
#include "../Admin/CheatCheck/CheatCheckManager.hpp"
#include "../Core/Managers.hpp"
#include "../Core/Permissions.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/App/Services.hpp>
#include <format>

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using AdminSystem::Admin::CheatCheck::CheatCheckManager;
using CS2Kit::Registry;
using CS2Kit::App::Engine;

namespace
{

const bool _registered = [] {
    Registry<CommandSpec>::Add({
        .Name = "cc",
        .Description = "Submit your verification link for a pending cheat check.",
        .Usage = "!cc <link>",
        .Args = {Word()},
        .Handler =
            [](CommandContext& c) {
                switch (App().CheatCheck.SubmitPlayerLink(c.CallerSlot(), c.Word))
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

    Registry<CommandSpec>::Add({
        .Name = "check",
        .Description = "Start a cheat check on a player.",
        .Usage = "!check <target>",
        .Permission = Flag(Permission::Control),
        .Args = {Target()},
        .Handler =
            [](CommandContext& c) {
                if (!AdminSystem::Admin::Actions::CallCheck(c.CallerSlot(), c.Target->GetSlot()))
                    return c.Fail("common.noPermission");
                return c.Ok("cheatCheck.started", {{"name", c.Target->GetName()}});
            },
    });

    Registry<CommandSpec>::Add({
        .Name = "cccancel",
        .Aliases = {"uncheck"},
        .Description = "Cancel a pending cheat check on a player.",
        .Usage = "!cccancel <target>",
        .Permission = Flag(Permission::Control),
        .Args = {Target()},
        .Handler =
            [](CommandContext& c) {
                if (!AdminSystem::Admin::Actions::CancelCheck(c.CallerSlot(), c.Target->GetSlot()))
                    return c.Fail("cheatCheck.noActiveCheck");
                return CommandResult{
                    true, std::format("{} {}", Engine().Utils.Translations.Get("cheatCheck.cancelled", c.CallerSlot()),
                                      c.Target->GetName())};
            },
    });

    return true;
}();

}  // namespace

}  // namespace AdminSystem::Commands
