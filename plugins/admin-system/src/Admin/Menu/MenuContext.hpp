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
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace AdminSystem::Admin::Menu
{

/** One admin's view of the panel: the plugin, plus the admin every builder is building for,
 *  resolved once here rather than in each of them. */
struct MenuContext
{
    /** Empty when the admin is no longer connected, which is the only reason a build fails. */
    [[nodiscard]] static std::optional<MenuContext> For(App& plugin, int adminSlot)
    {
        VoltMod::Player* admin = plugin.Runtime.Players.Get(adminSlot);
        if (!admin)
            return std::nullopt;
        return MenuContext{plugin, admin->Ref()};
    }

    [[nodiscard]] std::string Translate(std::string_view key, const VoltMod::Tokens& tokens = {}) const
    {
        return Plugin.Runtime.Translations.Get(key, Admin.Slot, tokens);
    }

    [[nodiscard]] bool Visible(const RowSpec& row) const { return Menu::Visible(Plugin, Admin.Slot, row); }

    [[nodiscard]] bool AnyVisible(std::span<const RowSpec> rows) const
    {
        return Menu::AnyVisible(Plugin, Admin.Slot, rows);
    }

    /** The action/effect rows for this admin against @p target. */
    [[nodiscard]] VoltMod::ActionRows Rows(VoltMod::PlayerRef target) const { return Plugin.MenuRows(Admin, target); }

    /** The connected player @p target names, or null once they leave. */
    [[nodiscard]] VoltMod::Player* Player(VoltMod::PlayerRef target) const { return Plugin.Runtime.Players.Get(target); }

    App& Plugin;
    VoltMod::PlayerRef Admin;
};

/** Appends every row of @p specs this admin may see, in catalog order, built by @p make. A row
 *  @p make returns nothing for is skipped and logged: the catalog and a tab's `MakeRow` are two
 *  edit sites. */
template <class Make>
void AppendCatalogRows(const MenuContext& ctx, VoltMod::MenuBuilder& builder, std::span<const RowSpec> specs,
                       Make make)
{
    for (const RowSpec& spec : specs)
    {
        if (!ctx.Visible(spec))
            continue;

        VoltMod::MenuItem item = make(spec.Id);
        if (!item.Describe)
        {
            VoltMod::Log::Warn("Admin menu row '{}' is in the catalog but nothing builds it.", spec.LabelKey);
            continue;
        }
        builder.Add(std::move(item));
    }
}

}  // namespace AdminSystem::Admin::Menu
