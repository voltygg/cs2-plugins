#include "../Core/App.hpp"
#include "../Core/ChatService.hpp"
#include "../Core/Permissions.hpp"
#include "../Maps/MapCycleState.hpp"
#include "Commands.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>

namespace AdminSystem::Commands
{

using namespace VoltMod::Commands;
using AdminSystem::Maps::MapMatch;
using VoltMod::Tokens;

namespace
{

/** Seconds players get to read the announcement before the server drops them into the load. */
constexpr int64_t ChangeAnnounceMs = 5000;

/** Resolve a typed map name, replying with the reason when it does not land on exactly one. */
const Maps::MapEntry* Resolve(App& app, CommandContext& c, std::string_view query)
{
    auto lookup = app.MapCycle.Find(query);
    Tokens tokens{{"map", std::string(query)}};

    switch (lookup.Result)
    {
    case MapMatch::Unique:
        return &app.MapCycle.Cycle()[lookup.Index];
    case MapMatch::Ambiguous:
        tokens["count"] = std::to_string(lookup.Count);
        c.Fail("cmd.mapAmbiguous", tokens);
        return nullptr;
    case MapMatch::None:
        c.Fail("cmd.mapNoMatch", tokens);
        return nullptr;
    }
    return nullptr;
}

}  // namespace

void RegisterMapCommands(VoltMod::CommandManager& commands, App& app)
{
    commands.Register({
        .Name = "map",
        .Description = "Change to a configured map after a short announcement.",
        .Permission = Flag(Permission::Map),
        .Args = {Word()},
        .Handler =
            [&app](CommandContext& c) {
                const auto* map = Resolve(app, c, c.Word);
                if (!map)
                    return CommandResult::Silent();

                app.Chat.BroadcastKey("broadcast.mapChanging", {{"map", map->Label()}});
                app.MapCycle.ChangeAfter(*map, ChangeAnnounceMs);
                return CommandResult::Silent();
            },
    });

    commands.Register({
        .Name = "maps",
        .Aliases = {"maplist"},
        .Description = "List the maps this server can switch to.",
        .Permission = Flag(Permission::Map),
        .Handler =
            [&app](CommandContext& c) {
                const auto& cycle = app.MapCycle.Cycle();
                if (cycle.empty())
                    return c.Fail("cmd.mapCycleEmpty");

                int slot = c.CallerSlot();
                app.Chat.Reply(slot, app.Runtime.Translations.Get("cmd.mapListHeader", slot,
                                                                  {{"count", std::to_string(cycle.size())}}));
                for (const auto& map : cycle)
                    app.Chat.Reply(slot, std::format("  {}", map.Label()));
                return CommandResult::Silent();
            },
    });

    commands.Register({
        .Name = "nextmap",
        .Description = "Show the map queued for the end of this round.",
        .Handler =
            [&app](CommandContext& c) {
                const auto& next = app.MapCycle.Next();
                return next ? c.Ok("cmd.nextMap", {{"map", next->Label()}}) : c.Ok("cmd.nextMapUnset");
            },
    });

    commands.Register({
        .Name = "setnextmap",
        .Description = "Queue a map for the end of the current round.",
        .Permission = Flag(Permission::Map),
        .Args = {Word()},
        .Handler =
            [&app](CommandContext& c) {
                const auto* map = Resolve(app, c, c.Word);
                if (!map)
                    return CommandResult::Silent();

                app.MapCycle.SetNext(*map);
                app.Chat.BroadcastKey("broadcast.nextMapSet", {{"map", map->Label()}});
                return CommandResult::Silent();
            },
    });
}

}  // namespace AdminSystem::Commands
