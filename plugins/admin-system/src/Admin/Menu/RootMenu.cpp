#include "Admin/Menu/RootMenu.hpp"

#include "Admin/Menu/MenuCatalog.hpp"
#include "Admin/Menu/MenuContext.hpp"
#include "Admin/Menu/Tabs/MapVoteTab.hpp"
#include "Admin/Menu/Tabs/MySettingsTab.hpp"
#include "Admin/Menu/Tabs/PlayerActionsTab.hpp"
#include "Admin/Menu/Tabs/PlayerFunTab.hpp"
#include "Admin/Menu/Tabs/PunishTab.hpp"
#include "Admin/Menu/Tabs/RoundModesTab.hpp"
#include "Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>
#include <memory>
#include <optional>
#include <string>

namespace AdminSystem::Admin::Menu
{

using VoltMod::MenuBuilder;
using VoltMod::SubmenuRow;

/** The builder behind one tab. No `default:`, so a new tab cannot be added without one. */
static std::shared_ptr<VoltMod::Menu> BuildTab(const MenuContext& ctx, TabId id)
{
    switch (id)
    {
    case TabId::Punish:
        return BuildPunishTab(ctx);
    case TabId::PlayerActions:
        return BuildPlayerActionsTab(ctx);
    case TabId::PlayerFun:
        return BuildPlayerFunTab(ctx);
    case TabId::RoundModes:
        return BuildRoundModesTab(ctx);
    case TabId::MapVote:
        return BuildMapVoteTab(ctx);
    case TabId::MySettings:
        return BuildMySettingsTab(ctx);
    }
    return nullptr;
}

std::shared_ptr<VoltMod::Menu> BuildRootMenu(AdminSystem::App& app, int adminSlot)
{
    const std::optional<MenuContext> ctx = MenuContext::For(app, adminSlot);
    if (!ctx)
        return nullptr;

    // The version goes in the subtitle rather than into the title as markup: both menu hosts
    // show a subtitle, and only one of them can render a <font> tag.
    MenuBuilder builder(ctx->Translate("panel.admin"));
    builder.Subtitle(std::format("v{}", app.Runtime.Version));

    for (const TabSpec& tab : Tabs)
    {
        // A tab is worth a slot only while something inside it is. Derived from the rows
        // themselves, so a tab can no longer outlive its contents the way a hand-kept list did.
        if (!ctx->AnyVisible(tab.Rows))
            continue;

        builder.Add(SubmenuRow{.Label = ctx->Translate(tab.LabelKey),
                               .Build = [ctx = *ctx, id = tab.Id](int) { return BuildTab(ctx, id); },
                               .Icon = std::string(tab.Icon)});
    }

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
