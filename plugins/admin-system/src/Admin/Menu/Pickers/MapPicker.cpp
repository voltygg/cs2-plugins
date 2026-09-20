#include "Admin/Menu/Pickers/MapPicker.hpp"

#include "Admin/Menu/Flows/MapChangeFlow.hpp"
#include "Core/App.hpp"
#include "Core/ChatService.hpp"
#include "Maps/MapCycleState.hpp"
#include "Maps/VoteState.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Admin::Menu
{

using AdminSystem::Maps::MapEntry;
using VoltMod::MenuBuilder;

/** The title key each verb's list carries, so the screen says what picking a map will do. */
static std::string_view TitleKey(MapVerb verb)
{
    switch (verb)
    {
    case MapVerb::SetNext:
        return "action.setNextMap";
    case MapVerb::PutToVote:
        return "action.voteMap";
    case MapVerb::ChangeNow:
        break;
    }
    return "action.changeMap";
}

static void Apply(const MenuContext& ctx, MapVerb verb, const MapEntry& map, int slot)
{
    App& app = ctx.Plugin;
    switch (verb)
    {
    case MapVerb::ChangeNow:
        ConfirmMapChange(ctx, map);
        return;
    case MapVerb::SetNext:
        // Queuing takes effect at the end of the round, so it needs no confirmation step.
        app.MapCycle.SetNext(map);
        app.Chat.BroadcastAction("broadcast.nextMapSet", Core::ActorName(app.Runtime, slot), {{"map", map.Label()}});
        return;
    case MapVerb::PutToVote:
        if (!app.Votes.StartMapVote(map, slot))
            app.Chat.Reply(slot, app.Runtime.Translations.Get("cmd.voteInProgress", slot));
        return;
    }
}

std::shared_ptr<VoltMod::Menu> BuildMapPicker(const MenuContext& ctx, MapVerb verb)
{
    MenuBuilder builder(ctx.Translate(TitleKey(verb)));
    builder.EmptyText(ctx.Translate("map.noMaps"));

    for (const auto& map : ctx.Plugin.MapCycle.Cycle())
        builder.Button(map.Label(), [ctx, verb, map](int slot) { Apply(ctx, verb, map, slot); });

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
