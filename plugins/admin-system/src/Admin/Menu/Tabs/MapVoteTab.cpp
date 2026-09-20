#include "Admin/Menu/Tabs/MapVoteTab.hpp"

#include "Admin/Menu/Labels.hpp"
#include "Admin/Menu/MenuAccess.hpp"
#include "Core/App.hpp"
#include "Core/ChatService.hpp"
#include "Core/Permissions.hpp"
#include "Maps/MapCycleState.hpp"
#include "Maps/VoteState.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
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

/** Taking the server away from everyone confirms rather than firing on a single click. */
static void ConfirmMapChange(App& app, int adminSlot, MapEntry map)
{
    auto& translations = app.Runtime.Translations;

    VoltMod::Flow<MapEntry>::Create(app.Runtime.Menus, adminSlot, std::move(map))
        ->Validate(RequirePermission(app, Permission::Map, adminSlot))
        ->Confirm({.Title = ConfirmTitle(translations, "action.changeMap", adminSlot),
                   .Summary =
                       [&app, adminSlot](const MapEntry& m, VoltMod::SummaryRows& rows) {
                           rows.Add(app.Runtime.Translations.Get("map.name", adminSlot), m.Label());
                       }})
        ->Finish([&app, adminSlot](MapEntry& m) {
            app.Chat.BroadcastAction("broadcast.mapChanging", Core::ActorName(app.Runtime, adminSlot),
                                     {{"map", m.Label()}});
            app.MapCycle.ChangeAfter(m);
        })
        ->Begin();
}

/** What an admin can do with one map: switch now, queue it, or put it to the players. */
static std::shared_ptr<VoltMod::Menu> BuildMapActionsMenu(const MenuContext& ctx, const MapEntry& map)
{
    App& app = ctx.Plugin;
    const VoltMod::EnabledCondition mayMap = Allows(app, Permission::Map);
    const VoltMod::EnabledCondition mayVote = Allows(app, Permission::Vote);

    return MenuBuilder(map.Label())
        .Add(ButtonRow{.Label = ctx.Translate("action.changeMap"),
                       .Activate = [&app, map](int slot) { ConfirmMapChange(app, slot, map); },
                       .Enabled = mayMap})
        // Queuing and voting only take effect later, so neither needs a confirmation step.
        .Add(ButtonRow{.Label = ctx.Translate("action.setNextMap"),
                       .Activate =
                           [&app, map](int slot) {
                               app.MapCycle.SetNext(map);
                               app.Chat.BroadcastAction("broadcast.nextMapSet", Core::ActorName(app.Runtime, slot),
                                                        {{"map", map.Label()}});
                           },
                       .Enabled = mayMap})
        .Add(ButtonRow{.Label = ctx.Translate("action.voteMap"),
                       .Activate =
                           [&app, map](int slot) {
                               if (!app.Votes.StartMapVote(map, slot))
                                   app.Chat.Reply(slot, app.Runtime.Translations.Get("cmd.voteInProgress", slot));
                           },
                       .Enabled = mayVote})
        .Build();
}

std::shared_ptr<VoltMod::Menu> BuildMapVoteTab(const MenuContext& ctx)
{
    App& app = ctx.Plugin;
    MenuBuilder builder(ctx.Translate("category.map"));

    const auto& cycle = app.MapCycle.Cycle();
    for (const auto& map : cycle)
        builder.Submenu(map.Label(), [ctx, map](int) { return BuildMapActionsMenu(ctx, map); });

    // Not EmptyText: the cancel-vote row below is added either way, so the menu is never empty.
    if (cycle.empty())
        builder.Text(ctx.Translate("map.noMaps"));

    builder.Add(ButtonRow{
        .Label = ctx.Translate("action.cancelVote"),
        .Activate =
            [&app](int slot) {
                auto& translations = app.Runtime.Translations;
                app.Chat.Reply(
                    slot, translations.Get(app.Votes.CancelVote() ? "cmd.voteCancelled" : "cmd.noVoteRunning", slot));
            },
        .Enabled = Allows(app, Permission::Vote)});

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
