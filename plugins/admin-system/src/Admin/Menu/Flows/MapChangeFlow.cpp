#include "Admin/Menu/Flows/MapChangeFlow.hpp"

#include "Admin/Menu/Labels.hpp"
#include "Admin/Menu/MenuAccess.hpp"
#include "App.hpp"
#include "Core/ChatService.hpp"
#include "Core/Permissions.hpp"
#include "Maps/MapCycleState.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Menu/Flow.hpp>
#include <VoltMod/Runtime.hpp>
#include <utility>

namespace AdminSystem::Admin::Menu
{

using AdminSystem::Maps::MapEntry;

void ConfirmMapChange(const MenuContext& ctx, MapEntry map)
{
    App& app = ctx.Plugin;
    const int adminSlot = ctx.Admin.Slot;

    VoltMod::Flow<MapEntry>::Create(app.Runtime.Menus, adminSlot, std::move(map))
        ->Validate(RequirePermission(app, Permission::Map, adminSlot))
        ->Confirm({.Title = ConfirmTitle(app.Runtime.Translations, "action.changeMap", adminSlot),
                   .Summary = [ctx](const MapEntry& m,
                                    VoltMod::SummaryRows& rows) { rows.Add(ctx.Translate("map.name"), m.Label()); }})
        ->Finish([&app, adminSlot](MapEntry& m) {
            app.Chat.BroadcastAction("broadcast.mapChanging", Core::ActorName(app.Runtime, adminSlot),
                                     {{"map", m.Label()}});
            app.MapCycle.ChangeAfter(m);
        })
        ->Begin();
}

}  // namespace AdminSystem::Admin::Menu
