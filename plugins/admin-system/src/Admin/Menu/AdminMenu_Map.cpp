#include "AdminMenu_Map.hpp"

#include "../../Core/App.hpp"
#include "../../Core/ChatService.hpp"
#include "../../Core/Permissions.hpp"
#include "../../Maps/MapCycleState.hpp"
#include "../../Maps/VoteState.hpp"
#include "Labels.hpp"
#include "MenuAccess.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/Flow.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Runtime.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace AdminSystem::Admin::Menu
{

using AdminSystem::Maps::MapEntry;
using VoltMod::ButtonRow;
using VoltMod::MenuBuilder;

// Rows are re-checked per click through MayUse: changing and queuing a map need the map flag,
// starting and cancelling a vote the vote flag - the same split the commands had.

/** Taking the server away from everyone confirms rather than firing on a single click. */
static void ConfirmMapChange(App& app, int adminSlot, MapEntry map)
{
    auto& tr = app.Runtime.Translations;

    VoltMod::Flow<MapEntry>::Create(app.Runtime.Menus, adminSlot, std::move(map))
        ->Validate(RequirePermission(app, Permission::Map, adminSlot))
        ->Confirm({.Title = ConfirmTitle(tr, "action.changeMap", adminSlot),
                   .Summary =
                       [&app, adminSlot](const MapEntry& m) {
                           std::vector<std::pair<std::string, std::string>> rows;
                           rows.emplace_back(app.Runtime.Translations.Get("map.name", adminSlot), m.Label());
                           return rows;
                       },
                   .ConfirmLabel = ConfirmLabel(tr, adminSlot),
                   .CancelLabel = CancelLabel(tr, adminSlot)})
        ->Finish([&app](MapEntry& m) {
            app.Chat.BroadcastKey("broadcast.mapChanging", {{"map", m.Label()}});
            app.MapCycle.ChangeAfter(m);
        })
        ->Start();
}

/** What an admin can do with one map: switch now, queue it, or put it to the players. */
static std::shared_ptr<VoltMod::Menu> BuildMapActionsMenu(App& app, int adminSlot, const MapEntry& map)
{
    auto& tr = app.Runtime.Translations;

    const bool mayMap = MayUse(app, adminSlot, Permission::Map);
    const bool mayVote = MayUse(app, adminSlot, Permission::Vote);

    return MenuBuilder(map.Label())
        .Add(ButtonRow{.Label = tr.Get("action.changeMap", adminSlot),
                       .Activate = [&app, map](int slot) { ConfirmMapChange(app, slot, map); },
                       .Enabled = mayMap})
        // Queuing and voting only take effect later, so neither needs a confirmation step.
        .Add(ButtonRow{.Label = tr.Get("action.setNextMap", adminSlot),
                       .Activate =
                           [&app, map](int slot) {
                               if (!MayUse(app, slot, Permission::Map))
                                   return;
                               app.MapCycle.SetNext(map);
                               app.Chat.BroadcastKey("broadcast.nextMapSet", {{"map", map.Label()}});
                           },
                       .Enabled = mayMap})
        .Add(ButtonRow{.Label = tr.Get("action.voteMap", adminSlot),
                       .Activate =
                           [&app, map](int slot) {
                               if (!MayUse(app, slot, Permission::Vote))
                                   return;
                               if (!app.Votes.StartMapVote(map, slot))
                                   app.Chat.Reply(slot, app.Runtime.Translations.Get("cmd.voteInProgress", slot));
                           },
                       .Enabled = mayVote})
        .Build();
}

std::shared_ptr<VoltMod::Menu> BuildMapMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;

    MenuBuilder builder(tr.Get("category.map", adminSlot));

    const auto& cycle = app.MapCycle.Cycle();
    for (const auto& map : cycle)
        builder.Submenu(map.Label(), [&app, map](int slot) { return BuildMapActionsMenu(app, slot, map); });

    // Never show a dead-end empty page.
    if (cycle.empty())
        builder.Add(ButtonRow{.Label = tr.Get("map.noMaps", adminSlot), .Enabled = false});

    builder.Add(ButtonRow{
        .Label = tr.Get("action.cancelVote", adminSlot),
        .Activate =
            [&app](int slot) {
                if (!MayUse(app, slot, Permission::Vote))
                    return;
                auto& translations = app.Runtime.Translations;
                app.Chat.Reply(
                    slot, translations.Get(app.Votes.CancelVote() ? "cmd.voteCancelled" : "cmd.noVoteRunning", slot));
            },
        .Enabled = MayUse(app, adminSlot, Permission::Vote)});

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
