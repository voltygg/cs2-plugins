#include "AdminMenu_Effects.hpp"

#include "../../Core/App.hpp"
#include "../Actions/Descriptors.hpp"
#include "../AdminManager.hpp"
#include "../Effects/Descriptors.hpp"
#include "PlayerPicker.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Menu/MenuManager.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>

namespace AdminSystem::Admin::Menu
{

using VoltMod::ButtonRow;
using VoltMod::EffectDescriptor;
using VoltMod::MenuBuilder;

std::shared_ptr<VoltMod::Menu> BuildEffectsMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;
    return BuildPlayerPicker(
        app, adminSlot, {.Title = tr.Get("category.effects", adminSlot), .Pick = [&app, adminSlot](int targetSlot) {
                             auto& players = app.Runtime.Players;
                             auto actions =
                                 BuildEffectsActionsMenu(app, players.RefFor(adminSlot), players.RefFor(targetSlot));
                             if (actions)
                                 app.Runtime.Menus.Open(adminSlot, actions);
                         }});
}

std::shared_ptr<VoltMod::Menu> BuildEffectsActionsMenu(AdminSystem::App& app, VoltMod::PlayerRef admin,
                                                       VoltMod::PlayerRef target)
{
    auto& tr = app.Runtime.Translations;

    auto* adminPlayer = app.Runtime.Players.Get(admin);
    auto* targetPlayer = app.Runtime.Players.Get(target);
    if (!targetPlayer || !adminPlayer)
        return nullptr;

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.effects", admin.Slot), targetPlayer->Name()));
    auto rows = app.MenuRows(admin, target);
    bool hasS = rows.Allowed(Flag(Permission::Control));

    for (const EffectDescriptor* effect : app.EffectDescriptors.MenuEffects)
    {
        if (effect->Choices)
            builder.Add(rows.EffectPicker(*effect));
        else
            builder.Add(rows.Effect(*effect));
    }

    builder.Add(rows.Action("action.slap", app.ActionDescriptors.Slap))
        .Add(rows.Action("action.smite", app.ActionDescriptors.Smite));

    // Swap opens a second player picker, then runs the dual-target Swap.
    builder.Add(ButtonRow{.Label = rows.Tr("action.swap"),
                          .Activate =
                              [&app, admin, target](int slot) {
                                  auto picker = BuildPlayerPicker(
                                      app, admin.Slot,
                                      {.Title = app.Runtime.Translations.Get("common.selectSwapTarget", admin.Slot),
                                       .Pick =
                                           [&app, viewerSlot = admin.Slot, first = target](int secondSlot) {
                                               auto& players = app.Runtime.Players;
                                               Actions::Swap(app, players.RefFor(viewerSlot), first,
                                                             players.RefFor(secondSlot));
                                               app.Runtime.Menus.CloseAll(viewerSlot);
                                           },
                                       .Enabled =
                                           [&entities = app.Runtime.Entities, first = target](int candidate) {
                                               // Gray out partners Swap would reject: the already-picked player and the
                                               // dead.
                                               VoltMod::Pawn pawn = entities.PawnOf(candidate);
                                               return candidate != first.Slot && pawn && pawn.IsAlive();
                                           }});
                                  if (picker)
                                      app.Runtime.Menus.Open(slot, picker);
                              },
                          .Enabled = hasS});

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
