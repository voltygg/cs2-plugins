#include "CheatCheckCommands.hpp"

#include "../Admin/Actions/Descriptors.hpp"
#include "../Admin/CheatCheck/CheatCheckManager.hpp"
#include "../Core/Managers.hpp"
#include "../Core/Permissions.hpp"
#include "CommandHelpers.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <format>

using CS2Kit::Core::Engine;

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using namespace CS2Kit::Players;
using namespace AdminSystem::Commands::Helpers;
using AdminSystem::Admin::CheatCheck::CheatCheckManager;

namespace
{

CommandResult HandleCheatCheckLink(Player* caller, const std::vector<std::string>& args)
{
    int slot = caller->GetSlot();
    auto& tr = Engine().Translations;

    switch (App().CheatCheck.SubmitPlayerLink(slot, args[0]))
    {
    case CheatCheckManager::SubmitResult::Relayed:
        return {true, tr.Get("cheatCheck.linkReceived", slot)};
    case CheatCheckManager::SubmitResult::Invalid:
        return {false, tr.Get("cheatCheck.linkInvalid", slot)};
    case CheatCheckManager::SubmitResult::NoActiveCheck:
    default:
        return {false, tr.Get("cheatCheck.noActiveCheck", slot)};
    }
}

CommandResult HandleCheatCheckCancel(Player* admin, const std::vector<std::string>& args)
{
    std::string err;
    Player* target = ResolveSingle(args[0], admin, err);
    if (!target)
        return {false, err};

    int slot = admin->GetSlot();
    auto& tr = Engine().Translations;

    if (!AdminSystem::Admin::Actions::CancelCheck(slot, target->GetSlot()))
        return {false, tr.Get("cheatCheck.noActiveCheck", slot)};

    return {true, std::format("{} {}", tr.Get("cheatCheck.cancelled", slot), target->GetName())};
}

}  // namespace

void RegisterCheatCheckCommands(CommandManager& mgr)
{
    mgr.Register(CommandBuilder("cc")
                     .WithDescription("Submit your verification link for a pending cheat check.")
                     .WithUsage("!cc <link>")
                     .WithArgs(1, 1)
                     .OnExecute(HandleCheatCheckLink)
                     .Build());

    mgr.Register(CommandBuilder("cccancel")
                     .WithDescription("Cancel a pending cheat check on a player.")
                     .WithUsage("!cccancel <target>")
                     .RequirePermission(Flag(Permission::CheatCheck))
                     .WithArgs(1, 1)
                     .OnExecute(HandleCheatCheckCancel)
                     .Build());
}

}  // namespace AdminSystem::Commands
