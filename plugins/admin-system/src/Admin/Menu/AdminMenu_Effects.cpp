#include "AdminMenu_Effects.hpp"

#include "../../Core/App.hpp"
#include "../Actions/Descriptors.hpp"
#include "../AdminManager.hpp"
#include "../Effects/EffectRegistry.hpp"
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

    auto* target = app.Runtime.Players.Get(targetSlot);
    if (!target || !app.Runtime.Players.Get(adminSlot))
        return nullptr;

    VoltMod::MenuContext ctx{.Rt = &app.Runtime, .Admin = adminSlot, .Target = targetSlot, .Effects = &app.Effects};
    bool hasS = ctx.Allowed(Flag(Permission::Control));

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.effects", adminSlot), target->Name()));
    builder.WithContext(ctx);

    for (const auto& entry : Effects::MenuEffects)
    {
        if (entry.Toggle)
            builder.AddEffectToggleRow(*entry.Toggle);
        else if (entry.Param)
            builder.AddEffectPickerRow(*entry.Param);
    }

    builder.AddActionRow("action.slap", Actions::Slap).AddActionRow("action.smite", Actions::Smite);

    // Swap opens a second player picker, then runs the dual-target Swap.
    builder.AddButton(
        ctx.Tr("action.swap"),
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
