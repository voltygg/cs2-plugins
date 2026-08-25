#include "../Core/App.hpp"
#include "../Core/ChatService.hpp"
#include "../Core/Permissions.hpp"
#include "../Fun/FunMode.hpp"
#include "Commands.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>
#include <string>

namespace AdminSystem::Commands
{

using namespace VoltMod::Commands;
using AdminSystem::Fun::Toggle;

void RegisterFunCommands(VoltMod::CommandManager& commands, App& app)
{
    commands.Register({
        .Name = "fun",
        .Description = "Toggle a server-wide round modifier, or list them with no argument.",
        .Permission = Flag(Permission::FunMode),
        .Args = {Word(/*required*/ false)},
        .Handler =
            [&app](CommandContext& c) {
                auto& tr = app.Runtime.Translations;
                int slot = c.CallerSlot();

                if (c.Word.empty())
                {
                    app.Chat.Reply(slot, tr.Get("fun.listHeader", slot));
                    for (const auto& info : Fun::Toggles())
                    {
                        const char* stateKey = app.FunMode.IsOn(info.Id) ? "effectState.on" : "effectState.off";
                        app.Chat.Reply(slot,
                                       std::format("  {} - {} [{}]", Fun::ToggleWord(info.Id),
                                                   tr.Get(std::string(info.NameKey), slot), tr.Get(stateKey, slot)));
                    }
                    return CommandResult::Silent();
                }

                if (c.Word == "off" || c.Word == "clear")
                {
                    app.FunMode.ClearAll();
                    app.Chat.BroadcastKey("broadcast.funCleared");
                    return CommandResult::Silent();
                }

                Toggle toggle = Fun::ParseToggle(c.Word);
                if (toggle == Toggle::Count)
                    return c.Fail("fun.unknownToggle", {{"name", c.Word}});

                bool on = app.FunMode.Flip(toggle);
                for (const auto& info : Fun::Toggles())
                {
                    if (info.Id != toggle)
                        continue;
                    app.Chat.BroadcastKey(std::string(on ? info.OnKey : info.OffKey));
                    break;
                }
                return CommandResult::Silent();
            },
    });
}

}  // namespace AdminSystem::Commands
