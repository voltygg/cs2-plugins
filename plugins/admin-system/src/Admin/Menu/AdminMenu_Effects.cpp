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

std::shared_ptr<VoltMod::MenuView> BuildEffectsMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;
    return BuildPlayerPicker(
        app, adminSlot, tr.Get("category.effects", adminSlot), [&app](int viewerSlot, int targetSlot) {
            auto& players = app.Runtime.Players;
            auto actions = BuildEffectsActionsMenu(app, players.RefFor(viewerSlot), players.RefFor(targetSlot));
            if (actions)
                app.Runtime.Menus.OpenMenu(viewerSlot, actions);
        });
}

std::shared_ptr<VoltMod::MenuView> BuildEffectsActionsMenu(AdminSystem::App& app, VoltMod::PlayerRef admin,
                                                           VoltMod::PlayerRef target)
{
    auto& tr = app.Runtime.Translations;

    auto* adminPlayer = app.Runtime.Players.Get(admin);
    auto* targetPlayer = app.Runtime.Players.Get(target);
    if (!targetPlayer || !adminPlayer)
        return nullptr;

    MenuBuilder builder(app.Runtime.Menus,
                        std::format("{}: {}", tr.Get("category.effects", admin.Slot), targetPlayer->Name()));
    builder.For(admin, target, &app.Effects);
    bool hasS = builder.Allowed(Flag(Permission::Control));

    for (const EffectDescriptor* effect : app.EffectDescriptors.MenuEffects)
    {
        if (effect->Choices)
            builder.EffectPicker(*effect);
        else
            builder.Effect(*effect);
    }

    builder.Row("action.slap", app.ActionDescriptors.Slap).Row("action.smite", app.ActionDescriptors.Smite);

    // Swap opens a second player picker, then runs the dual-target Swap.
    builder.Button(
        builder.Tr("action.swap"),
        [&app, admin, target](int slot) {
            auto picker = BuildPlayerPicker(
                app, admin.Slot, app.Runtime.Translations.Get("common.selectSwapTarget", admin.Slot),
                [&app, first = target](int viewerSlot, int secondSlot) {
                    auto& players = app.Runtime.Players;
                    Actions::Swap(app, players.RefFor(viewerSlot), first, players.RefFor(secondSlot));
                    app.Runtime.Menus.CloseAllMenus(viewerSlot);
                },
                [&entities = app.Runtime.Entities, first = target](int candidate) {
                    // Gray out partners Swap would reject: the already-picked player and the dead.
                    VoltMod::Pawn pawn = entities.PawnOf(candidate);
                    return candidate != first.Slot && pawn && pawn.IsAlive();
                });
            if (picker)
                app.Runtime.Menus.OpenMenu(slot, picker);
        },
        hasS);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
