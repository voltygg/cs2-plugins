#include "Admin/Menu/Tabs/PlayerFunTab.hpp"

#include "Admin/Actions/Descriptors.hpp"
#include "Admin/AdminManager.hpp"
#include "Admin/Effects/Descriptors.hpp"
#include "Admin/Menu/Pickers/PlayerPicker.hpp"
#include "Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>

namespace AdminSystem::Admin::Menu
{

using VoltMod::EffectDescriptor;
using VoltMod::MenuBuilder;
using VoltMod::SubmenuRow;

std::shared_ptr<VoltMod::Menu> BuildPlayerFunTab(const MenuContext& ctx)
{
    return BuildPlayerPicker(ctx.Plugin, ctx.Admin.Slot,
                             {.Title = ctx.Translate("category.effects"),
                              .Open = [ctx](VoltMod::PlayerRef target) { return BuildPlayerFunCard(ctx, target); }});
}

std::shared_ptr<VoltMod::Menu> BuildPlayerFunCard(const MenuContext& ctx, VoltMod::PlayerRef target)
{
    App& app = ctx.Plugin;
    const VoltMod::PlayerRef admin = ctx.Admin;

    auto* targetPlayer = ctx.Player(target);
    if (!targetPlayer)
        return nullptr;

    MenuBuilder builder(std::format("{}: {}", ctx.Translate("category.effects"), targetPlayer->Name()));
    auto rows = ctx.Rows(target);

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
        SubmenuRow{.Label = rows.Translate("action.swap"),
                   .Build =
                       [ctx, &app, admin, target](int) {
                           return BuildPlayerPicker(
                               app, admin.Slot,
                               {.Title = ctx.Translate("common.selectSwapTarget"),
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
                   .Enabled = rows.Allows(Permission::Control)});

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
