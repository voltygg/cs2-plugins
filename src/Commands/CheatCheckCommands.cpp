#include "CheatCheckCommands.hpp"

#include "../Admin/Actions/CheatCheck.hpp"
#include "../Admin/CheatCheck/CheatCheckManager.hpp"
#include "CommandHelpers.hpp"

#include <CS2Kit/Utils/Translations.hpp>
#include <format>

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using namespace CS2Kit::Players;
using namespace AdminSystem::Commands::Helpers;
using AdminSystem::Admin::CheatCheck::CheatCheckManager;
using CS2Kit::Utils::Translations;

namespace
{

CommandResult HandleCheatCheckLink(Player* caller, const std::vector<std::string>& args)
{
    Translations::SlotScope scope(caller->GetSlot());
    auto& tr = Translations::Instance();

    switch (CheatCheckManager::Instance().SubmitPlayerLink(caller->GetSlot(), args[0]))
    {
    case CheatCheckManager::SubmitResult::Relayed:
        return {true, tr.Get("cheatCheck.linkReceived")};
    case CheatCheckManager::SubmitResult::Invalid:
        return {false, tr.Get("cheatCheck.linkInvalid")};
    case CheatCheckManager::SubmitResult::NoActiveCheck:
    default:
        return {false, tr.Get("cheatCheck.noActiveCheck")};
    }
}

CommandResult HandleCheatCheckCancel(Player* admin, const std::vector<std::string>& args)
{
    std::string err;
    Player* target = ResolveSingle(args[0], admin, err);
    if (!target)
        return {false, err};

    Translations::SlotScope scope(admin->GetSlot());
    auto& tr = Translations::Instance();

    if (!AdminSystem::Admin::Actions::DoCancelCheck(admin->GetSlot(), target->GetSlot()))
        return {false, tr.Get("cheatCheck.noActiveCheck")};

    return {true, std::format("{} {}", tr.Get("cheatCheck.cancelled"), target->GetName())};
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
                     .RequirePermission("k")
                     .WithArgs(1, 1)
                     .OnExecute(HandleCheatCheckCancel)
                     .Build());
}

}  // namespace AdminSystem::Commands
