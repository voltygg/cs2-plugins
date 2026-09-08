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

using VoltMod::EffectDescriptor;
using VoltMod::MenuBuilder;
using VoltMod::SubmenuRow;

std::shared_ptr<VoltMod::Menu> BuildEffectsMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;
    return BuildPlayerPicker(
        app, adminSlot,
        {.Title = tr.Get("category.effects", adminSlot), .Open = [&app, adminSlot](VoltMod::PlayerRef target) {
             return BuildEffectsActionsMenu(app, app.Runtime.Players.RefFor(adminSlot), target);
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

    for (const EffectDescriptor* effect : app.EffectDescriptors.MenuEffects)
    {
        if (effect->Choices)
            builder.Add(rows.EffectPicker(*effect));
        else
            builder.Add(rows.Effect(*effect));
    }

    builder.Add(rows.Action("action.slap", app.ActionDescriptors.Slap))
        .Add(rows.Action("action.smite", app.ActionDescriptors.Smite));

    // Swap opens a second player picker as a submenu, then runs the dual-target Swap.
    builder.Add(
        SubmenuRow{.Label = rows.Tr("action.swap"),
                   .Build =
                       [&app, admin, target](int) {
                           return BuildPlayerPicker(
                               app, admin.Slot,
                               {.Title = app.Runtime.Translations.Get("common.selectSwapTarget", admin.Slot),
                                .Pick =
                                    [&app, viewerSlot = admin.Slot, first = target](VoltMod::PlayerRef second) {
                                        Actions::Swap(app, app.Runtime.Players.RefFor(viewerSlot), first, second);
                                        app.Runtime.Menus.CloseAll(viewerSlot);
                                    },
                                .Enabled =
                                    [&entities = app.Runtime.Entities, first = target](VoltMod::PlayerRef candidate) {
                                        // Gray out partners Swap would reject: the already-picked player and the dead.
                                        VoltMod::Pawn pawn = entities.PawnOf(candidate.Slot);
                                        return candidate.Slot != first.Slot && pawn && pawn.IsAlive();
                                    }});
                       },
                   .Enabled = rows.Allows(Flag(Permission::Control))});

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
