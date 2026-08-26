#include "AdminMenu_Map.hpp"

#include "../../Core/App.hpp"
#include "../../Core/ChatService.hpp"
#include "../../Core/Permissions.hpp"
#include "../../Maps/MapCycleState.hpp"
#include "../../Maps/VoteState.hpp"
#include "MenuAccess.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/Flow.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace AdminSystem::Admin::Menu
{

using AdminSystem::Maps::MapEntry;
using VoltMod::MenuBuilder;

// Rows are re-checked per click through MayUse: changing and queuing a map need the map flag,
// starting and cancelling a vote the vote flag - the same split the commands had.

/** Taking the server away from everyone confirms rather than firing on a single click. */
static void ConfirmMapChange(App& app, int adminSlot, MapEntry map)
{
    VoltMod::Flow<MapEntry>::Create(app.Runtime.Menus, std::move(map))
        ->OnValidate([&app](int slot, const MapEntry&) -> std::optional<std::string> {
            if (!MayUse(app, slot, Permission::Map))
                return "punish.notAllowed";
            return std::nullopt;
        })
        ->WithConfirm(
            [&app](int slot) {
                auto& tr = app.Runtime.Translations;
                return std::format("{}: {}", tr.Get("punish.confirmTitle", slot), tr.Get("action.changeMap", slot));
            },
            [&app](int slot, const MapEntry& m) {
                std::vector<std::pair<std::string, std::string>> rows;
                rows.emplace_back(app.Runtime.Translations.Get("map.name", slot), m.Label());
                return rows;
            },
            [&app](int slot) { return app.Runtime.Translations.Get("punish.confirm", slot); },
            [&app](int slot) { return app.Runtime.Translations.Get("punish.cancel", slot); })
        ->OnFinish([&app](int, MapEntry& m) {
            app.Chat.BroadcastKey("broadcast.mapChanging", {{"map", m.Label()}});
            app.MapCycle.ChangeAfter(m);
        })
        ->Start(adminSlot);
}

/** What an admin can do with one map: switch now, queue it, or put it to the players. */
static std::shared_ptr<VoltMod::MenuView> BuildMapActionsMenu(App& app, int adminSlot, const MapEntry& map)
{
    auto& tr = app.Runtime.Translations;

    const bool mayMap = MayUse(app, adminSlot, Permission::Map);
    const bool mayVote = MayUse(app, adminSlot, Permission::Vote);

    return MenuBuilder(map.Label())
        .AddButton(
            tr.Get("action.changeMap", adminSlot), [&app, map](int slot) { ConfirmMapChange(app, slot, map); }, mayMap)
        // Queuing and voting only take effect later, so neither needs a confirmation step.
        .AddButton(
            tr.Get("action.setNextMap", adminSlot),
            [&app, map](int slot) {
                if (!MayUse(app, slot, Permission::Map))
                    return;
                app.MapCycle.SetNext(map);
                app.Chat.BroadcastKey("broadcast.nextMapSet", {{"map", map.Label()}});
            },
            mayMap)
        .AddButton(
            tr.Get("action.voteMap", adminSlot),
            [&app, map](int slot) {
                if (!MayUse(app, slot, Permission::Vote))
                    return;
                if (!app.Votes.StartMapVote(map, slot))
                    app.Chat.Reply(slot, app.Runtime.Translations.Get("cmd.voteInProgress", slot));
            },
            mayVote)
        .Build();
}

std::shared_ptr<VoltMod::MenuView> BuildMapMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;

    MenuBuilder builder(tr.Get("category.map", adminSlot));

    const auto& cycle = app.MapCycle.Cycle();
    for (const auto& map : cycle)
        builder.AddSubmenu(map.Label(), [&app, map](int slot) { return BuildMapActionsMenu(app, slot, map); });

    // Never show a dead-end empty page.
    if (cycle.empty())
        builder.AddButton(tr.Get("map.noMaps", adminSlot), [](int) {}, false);

    builder.AddButton(
        tr.Get("action.cancelVote", adminSlot),
        [&app](int slot) {
            if (!MayUse(app, slot, Permission::Vote))
                return;
            auto& translations = app.Runtime.Translations;
            app.Chat.Reply(slot,
                           translations.Get(app.Votes.CancelVote() ? "cmd.voteCancelled" : "cmd.noVoteRunning", slot));
        },
        MayUse(app, adminSlot, Permission::Vote));

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
