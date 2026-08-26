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

using VoltMod::MenuBuilder;

std::shared_ptr<VoltMod::MenuView> BuildEffectsMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;
    return BuildPlayerPicker(app, adminSlot, tr.Get("category.effects", adminSlot), [&app](int admin, int target) {
        auto actions = BuildEffectsActionsMenu(app, admin, target);
        if (actions)
            app.Runtime.Menus.OpenMenu(admin, actions);
    });
}

std::shared_ptr<VoltMod::MenuView> BuildEffectsActionsMenu(AdminSystem::App& app, int adminSlot, int targetSlot)
{
    auto& tr = app.Runtime.Translations;

    auto* admin = app.Runtime.Players.Get(adminSlot);
    auto* target = app.Runtime.Players.Get(targetSlot);
    if (!target || !admin)
        return nullptr;

    MenuBuilder builder(app.Runtime.Menus,
                        std::format("{}: {}", tr.Get("category.effects", adminSlot), target->Name()));
    builder.For(admin->Ref(), target->Ref(), &app.Effects);
    bool hasS = builder.Allowed(Flag(Permission::Control));

    for (const auto& entry : app.EffectDescriptors.MenuEffects)
    {
        if (entry.Toggle)
        {
            if (entry.Toggle->Choices)
                builder.EffectPicker(*entry.Toggle);
            else
                builder.Effect(*entry.Toggle);
        }
    }

    builder.Row("action.slap", app.ActionDescriptors.Slap).Row("action.smite", app.ActionDescriptors.Smite);

    // Swap opens a second player picker, then runs the dual-target Swap.
    builder.Button(
        builder.Tr("action.swap"),
        [&app, adminSlot, targetSlot](int slot) {
            auto picker = BuildPlayerPicker(
                app, adminSlot, app.Runtime.Translations.Get("common.selectSwapTarget", adminSlot),
                [&app, first = targetSlot](int a, int second) {
                    Actions::Swap(app, a, first, second);
                    app.Runtime.Menus.CloseAllMenus(a);
                },
                [&entities = app.Runtime.Entities, first = targetSlot](int candidate) {
                    // Gray out partners Swap would reject: the already-picked player and the dead.
                    VoltMod::Pawn pawn = entities.PawnOf(candidate);
                    return candidate != first && pawn && pawn.IsAlive();
                });
            if (picker)
                app.Runtime.Menus.OpenMenu(slot, picker);
        },
        hasS);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
