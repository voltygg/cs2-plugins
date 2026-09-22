#pragma once

#include "Admin/Menu/MenuAccess.hpp"
#include "Admin/Menu/MenuCatalog.hpp"
#include "Core/App.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Menu/ActionRows.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Players/PlayerRef.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace AdminSystem::Admin::Menu
{

/** One admin's view of the panel: the plugin, plus the admin every builder is building for,
 *  resolved once here rather than in each of them. */
struct MenuContext
{
    /** Empty when the admin is no longer connected, which is the only reason a build fails. */
    static std::optional<MenuContext> For(App& plugin, int adminSlot)
    {
        VoltMod::Player* admin = plugin.Runtime.Players.Get(adminSlot);
        if (!admin)
            return std::nullopt;
        return MenuContext{plugin, admin->Ref()};
    }

    std::string Translate(std::string_view key, const VoltMod::Tokens& tokens = {}) const
    {
        return Plugin.Runtime.Translations.Get(key, Admin.Slot, tokens);
    }

    bool Visible(const RowSpec& row) const { return Menu::Visible(Plugin, Admin.Slot, row); }

    bool AnyVisible(std::span<const RowSpec> rows) const { return Menu::AnyVisible(Plugin, Admin.Slot, rows); }

    bool Visible(const TabSpec& tab) const
    {
        return tab.Permission.empty() ? AnyVisible(tab.Rows) : MayUse(Plugin, Admin.Slot, tab.Permission);
    }

    /** The action/effect rows for this admin against @p target. */
    VoltMod::ActionRows Rows(VoltMod::PlayerRef target) const { return Plugin.MenuRows(Admin, target); }

    /** The connected player @p target names, or null once they leave. */
    VoltMod::Player* Player(VoltMod::PlayerRef target) const { return Plugin.Runtime.Players.Get(target); }

    /** @ref Menu::WhileAlive, applied only when @p descriptor refuses a dead target. */
    template <class Descriptor>
    VoltMod::MenuItem WhileAlive(VoltMod::PlayerRef target, const Descriptor& descriptor, VoltMod::MenuItem item) const
    {
        return descriptor.RequireAlive ? Menu::WhileAlive(Plugin, Admin.Slot, target, std::move(item))
                                       : std::move(item);
    }

    /** The "<tab>: <name>" title every card carries, or nothing once @p target leaves. */
    std::optional<std::string> CardTitle(std::string_view titleKey, VoltMod::PlayerRef target) const
    {
        VoltMod::Player* player = Player(target);
        if (!player)
            return std::nullopt;
        return std::format("{}: {}", Translate(titleKey), player->Name());
    }

    App& Plugin;
    VoltMod::PlayerRef Admin;
};

/** Appends every row of @p specs this admin may see, in catalog order, each built by @p make from
 *  its whole spec. A row @p make returns nothing for is skipped and logged. */
template <class Make>
void AppendCatalogRows(const MenuContext& ctx, VoltMod::MenuBuilder& builder, std::span<const RowSpec> specs, Make make)
{
    for (const RowSpec& spec : specs)
    {
        if (!ctx.Visible(spec))
            continue;

        VoltMod::MenuItem item = make(spec);
        if (!item.Describe)
        {
            VoltMod::Log::Warn("Admin menu row '{}' is in the catalog but nothing builds it.", spec.LabelKey);
            continue;
        }
        builder.Add(std::move(item));
    }
}

}  // namespace AdminSystem::Admin::Menu
