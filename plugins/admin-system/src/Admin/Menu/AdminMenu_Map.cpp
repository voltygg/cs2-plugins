#include "AdminMenu_Map.hpp"

#include "../../Core/App.hpp"
#include "../../Core/ChatService.hpp"
#include "../../Core/Permissions.hpp"
#include "../../Maps/MapCycleState.hpp"

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
using VoltMod::Menu::MenuBuilder;

namespace
{

/** Seconds players get to read the announcement before the server drops them into the load. */
constexpr int64_t ChangeAnnounceMs = 5000;

/** Changing map ends everyone's round, so it confirms rather than firing on a single click. */
void StartMapChangeConfirm(App& app, int adminSlot, MapEntry map)
{
    VoltMod::Flow<MapEntry>::Create(app.Runtime.Menus, std::move(map))
        // The Map flag may have been revoked (e.g. !admin_reload) while the menu was open.
        ->OnValidate([&app](int slot, const MapEntry&) -> std::optional<std::string> {
            auto* admin = app.Runtime.Players.GetPlayerBySlot(slot);
            if (!admin || !app.Access.HasPermission(admin->GetSteamID(), Permission::Map))
                return "punish.notAllowed";
            return std::nullopt;
        })
        ->WithConfirm(
            [&app](int slot) {
                auto& tr = app.Runtime.Translations;
                return std::format("{}: {}", tr.Get("punish.confirmTitle", slot), tr.Get("action.changeMap", slot));
            },
            [&app](int slot, const MapEntry& m) {
                auto& tr = app.Runtime.Translations;
                std::vector<std::pair<std::string, std::string>> rows;
                rows.emplace_back(tr.Get("map.name", slot), m.Label());
                return rows;
            },
            [&app](int slot) { return app.Runtime.Translations.Get("punish.confirm", slot); },
            [&app](int slot) { return app.Runtime.Translations.Get("punish.cancel", slot); })
        ->OnFinish([&app](int, MapEntry& m) {
            app.Chat.BroadcastKey("broadcast.mapChanging", {{"map", m.Label()}});
            app.MapCycle.ChangeAfter(m, ChangeAnnounceMs);
        })
        ->Start(adminSlot);
}

}  // namespace

std::shared_ptr<VoltMod::MenuView> BuildMapMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;

    MenuBuilder builder(tr.Get("category.map", adminSlot));

    const auto& cycle = app.MapCycle.Cycle();
    for (const auto& map : cycle)
        builder.AddButton(map.Label(), [&app, map](int slot) { StartMapChangeConfirm(app, slot, map); });

    // Never show a dead-end empty page.
    if (cycle.empty())
        builder.AddButton(tr.Get("map.noMaps", adminSlot), [](int) {}, false);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
