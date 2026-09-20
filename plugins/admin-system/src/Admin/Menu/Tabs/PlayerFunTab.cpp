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
#include <format>
#include <utility>

namespace AdminSystem::Admin::Menu
{

using VoltMod::MenuBuilder;

static constexpr int SizePresets[] = {10, 25, 50, 75, 100, 150, 200};

// Size cycles both up and down from normal, so it opens anchored on 100% (no change).
static constexpr int SizeDefault = 4;  // index of 100 in SizePresets

/** The row @p id names, for the target this card belongs to. */
static VoltMod::MenuItem MakeRow(const MenuContext& ctx, VoltMod::ActionRows& rows, RowId id,
                                VoltMod::PlayerRef target)
{
    auto& effects = ctx.Plugin.EffectDescriptors;
    auto& actions = ctx.Plugin.ActionDescriptors;

    // A RequireAlive descriptor is skipped silently on a dead target, so those rows say why.
    auto live = [&](const auto& descriptor, VoltMod::MenuItem item) {
        return descriptor.RequireAlive ? WhileAlive(ctx.Plugin, ctx.Admin.Slot, target, std::move(item))
                                       : std::move(item);
    };
    auto effect = [&](const VoltMod::EffectDescriptor& e) { return live(e, rows.Effect(e)); };

    switch (id)
    {
    case RowId::Ghost:
        return effect(effects.Ghost);
    case RowId::Disco:
        return effect(effects.Disco);
    case RowId::Wallhack:
        return effect(effects.Wallhack);
    case RowId::Model:
        return live(effects.Model, rows.EffectPicker(effects.Model));
    case RowId::Bhop:
        return effect(effects.Bhop);
    case RowId::Drunk:
        return effect(effects.Drunk);
    case RowId::Smite:
        return live(actions.Smite, rows.Action("action.smite", actions.Smite));
    case RowId::Size:
        return live(actions.SetSize, rows.Presets({.LabelKey = "action.size",
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
    auto* targetPlayer = ctx.Player(target);
    if (!targetPlayer)
        return nullptr;

    MenuBuilder builder(std::format("{}: {}", ctx.Translate("category.playerFun"), targetPlayer->Name()));
    auto rows = ctx.Rows(target);

    AppendCatalogRows(ctx, builder, PlayerFunRows,
                      [&](RowId id) { return MakeRow(ctx, rows, id, target); });

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
