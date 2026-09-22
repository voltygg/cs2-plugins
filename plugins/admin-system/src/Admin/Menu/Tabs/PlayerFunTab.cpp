#include "Admin/Menu/Tabs/PlayerFunTab.hpp"

#include "Admin/Actions/Descriptors.hpp"
#include "Admin/Effects/Descriptors.hpp"
#include "Admin/Menu/MenuAccess.hpp"
#include "Admin/Menu/MenuCatalog.hpp"
#include "Admin/Menu/Pickers/PlayerPicker.hpp"
#include "Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/ActionRows.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>

namespace AdminSystem::Admin::Menu
{

using VoltMod::MenuBuilder;

static constexpr int SizePresets[] = {10, 25, 50, 75, 100, 150, 200};

// Size cycles both up and down from normal, so it opens anchored on 100% (no change).
static constexpr int SizeDefault = 4;  // index of 100 in SizePresets

static VoltMod::MenuItem MakeRow(const MenuContext& ctx, VoltMod::ActionRows& rows, const RowSpec& spec,
                                 VoltMod::PlayerRef target)
{
    auto& effects = ctx.Plugin.EffectDescriptors;
    auto& actions = ctx.Plugin.ActionDescriptors;

    auto effect = [&](const VoltMod::EffectDescriptor& e) { return ctx.WhileAlive(target, e, rows.Effect(e)); };

    switch (spec.Id)
    {
    case RowId::Ghost:
        return effect(effects.Ghost);
    case RowId::Disco:
        return effect(effects.Disco);
    case RowId::Wallhack:
        return effect(effects.Wallhack);
    case RowId::Model:
        return ctx.WhileAlive(target, effects.Model, rows.EffectPicker(effects.Model));
    case RowId::Bhop:
        return effect(effects.Bhop);
    case RowId::Drunk:
        return effect(effects.Drunk);
    case RowId::Smite:
        return ctx.WhileAlive(target, actions.Smite, rows.Action(spec.LabelKey, actions.Smite));
    case RowId::Size:
        return ctx.WhileAlive(target, actions.SetSize,
                              rows.Presets({.LabelKey = spec.LabelKey,
                                            .Unit = "%",
                                            .Presets = SizePresets,
                                            .Action = actions.SetSize,
                                            .Index = SizeDefault}));
    default:
        return {};
    }
}

std::shared_ptr<VoltMod::Menu> BuildPlayerFunTab(const MenuContext& ctx)
{
    MenuBuilder builder(ctx.Translate("category.playerFun"));
    AppendTargetRows(ctx, builder, [ctx](VoltMod::PlayerRef target) { return BuildPlayerFunCard(ctx, target); });
    return builder.Build();
}

std::shared_ptr<VoltMod::Menu> BuildPlayerFunCard(const MenuContext& ctx, VoltMod::PlayerRef target)
{
    const auto title = ctx.CardTitle("category.playerFun", target);
    if (!title)
    {
        return nullptr;
    }

    MenuBuilder builder(*title);
    auto rows = ctx.Rows(target);

    AppendCatalogRows(ctx, builder, PlayerFunRows,
                      [&](const RowSpec& spec) { return MakeRow(ctx, rows, spec, target); });

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
