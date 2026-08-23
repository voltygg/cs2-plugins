#include "AdminMenu_Effects.hpp"

#include "../../Core/App.hpp"
#include "../Actions/Descriptors.hpp"
#include "../AdminManager.hpp"
#include "../Effects/EffectRegistry.hpp"
#include "PlayerPicker.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Runtime.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <algorithm>
#include <format>
#include <vector>

namespace AdminSystem::Admin::Menu
{

using CS2Kit::Menu::MenuBuilder;

std::shared_ptr<CS2Kit::MenuView> BuildEffectsMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;
    return BuildPlayerPicker(app, adminSlot, tr.Get("category.effects", adminSlot), [&app](int admin, int target) {
        auto actions = BuildEffectsActionsMenu(app, admin, target);
        if (actions)
            app.Runtime.Menus.OpenMenu(admin, actions);
    });
}

std::shared_ptr<CS2Kit::MenuView> BuildEffectsActionsMenu(AdminSystem::App& app, int adminSlot, int targetSlot)
{
    auto& tr = app.Runtime.Translations;

    auto* target = app.Runtime.Players.GetPlayerBySlot(targetSlot);
    if (!target || !app.Runtime.Players.GetPlayerBySlot(adminSlot))
        return nullptr;

    CS2Kit::MenuContext ctx{.Admin = adminSlot, .Target = targetSlot, .Effects = &app.Effects};
    bool hasS = ctx.Allowed(Flag(Permission::Control));

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.effects", adminSlot), target->GetName()));
    builder.WithContext(ctx);

    std::vector<Effects::EffectEntry> entries(Effects::MenuEffects.begin(), Effects::MenuEffects.end());
    std::ranges::sort(entries, {}, &Effects::EffectEntry::Order);
    for (const auto& entry : entries)
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
                [first = targetSlot](int candidate) {
                    // Gray out partners Swap would reject: the already-picked player and the dead.
                    CS2Kit::PlayerController ctrl(candidate);
                    return candidate != first && ctrl.IsValid() && ctrl.IsAlive();
                });
            if (picker)
                app.Runtime.Menus.OpenMenu(slot, picker);
        },
        hasS);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
