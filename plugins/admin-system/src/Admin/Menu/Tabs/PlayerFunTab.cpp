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

static constexpr int SizePresets[] = {10, 25, 50, 75, 100, 150, 200};

// Size cycles both up and down from normal, so it opens anchored on 100% (no change).
static constexpr int SizeDefault = 4;  // index of 100 in SizePresets

std::shared_ptr<VoltMod::Menu> BuildPlayerFunTab(const MenuContext& ctx)
{
    return BuildPlayerPicker(ctx.Plugin, ctx.Admin.Slot,
                             {.Title = ctx.Translate("category.playerFun"),
                              .Open = [ctx](VoltMod::PlayerRef target) { return BuildPlayerFunCard(ctx, target); }});
}

std::shared_ptr<VoltMod::Menu> BuildPlayerFunCard(const MenuContext& ctx, VoltMod::PlayerRef target)
{
    App& app = ctx.Plugin;
    auto* targetPlayer = ctx.Player(target);
    if (!targetPlayer)
        return nullptr;

    MenuBuilder builder(std::format("{}: {}", ctx.Translate("category.playerFun"), targetPlayer->Name()));
    auto rows = ctx.Rows(target);

    for (const EffectDescriptor* effect : app.EffectDescriptors.MenuEffects)
    {
        if (effect->Choices)
            builder.Add(rows.EffectPicker(*effect));
        else
            builder.Add(rows.Effect(*effect));
    }

    builder.Add(rows.Action("action.smite", app.ActionDescriptors.Smite))
        .Add(rows.Presets({.LabelKey = "action.size",
                           .Unit = "%",
                           .Presets = SizePresets,
                           .Action = app.ActionDescriptors.SetSize,
                           .Index = SizeDefault}));

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
