#include "AdminMenu_Effects.hpp"

#include "../../Core/Managers.hpp"
#include "../Actions/Descriptors.hpp"
#include "../AdminManager.hpp"
#include "../Effects/EffectRegistry.hpp"
#include "PlayerPicker.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <algorithm>
#include <format>
#include <vector>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Menu
{

using CS2Kit::Menu::MenuBuilder;

std::shared_ptr<CS2Kit::MenuView> BuildEffectsMenu(int adminSlot)
{
    auto& tr = Engine().Translations;
    return BuildPlayerPicker(adminSlot, tr.Get("category.effects", adminSlot), [](int admin, int target) {
        auto actions = BuildEffectsActionsMenu(admin, target);
        if (actions)
            Engine().Menus.OpenMenu(admin, actions);
    });
}

std::shared_ptr<CS2Kit::MenuView> BuildEffectsActionsMenu(int adminSlot, int targetSlot)
{
    auto& tr = Engine().Translations;

    auto* target = Engine().Players.GetPlayerBySlot(targetSlot);
    if (!target || !Engine().Players.GetPlayerBySlot(adminSlot))
        return nullptr;

    CS2Kit::MenuContext ctx{.Admin = adminSlot, .Target = targetSlot, .Effects = &App().Effects};
    bool hasS = ctx.Allowed(Flag(Permission::Control));

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.effects", adminSlot), target->GetName()));
    builder.WithContext(ctx);

    auto registered = CS2Kit::Registry<Effects::EffectEntry>::Items();
    std::vector<Effects::EffectEntry> entries(registered.begin(), registered.end());
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
        [adminSlot, targetSlot](int slot) {
            auto picker = BuildPlayerPicker(
                adminSlot, Engine().Translations.Get("common.selectSwapTarget", adminSlot),
                [first = targetSlot](int a, int second) {
                    Actions::Swap(a, first, second);
                    Engine().Menus.CloseAllMenus(a);
                },
                [first = targetSlot](int candidate) {
                    // Gray out partners Swap would reject: the already-picked player and the dead.
                    CS2Kit::PlayerController ctrl(candidate);
                    return candidate != first && ctrl.IsValid() && ctrl.IsAlive();
                });
            if (picker)
                Engine().Menus.OpenMenu(slot, picker);
        },
        hasS);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
