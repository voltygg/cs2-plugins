#pragma once

#include "Admin/Menu/MenuAccess.hpp"
#include "Admin/Menu/MenuCatalog.hpp"
#include "Core/App.hpp"

#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Menu/ActionRows.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Players/PlayerRef.hpp>
#include <VoltMod/Runtime.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace AdminSystem::Admin::Menu
{

/**
 * @brief One admin's view of the panel: the plugin plus the admin every builder is building for.
 *
 * Resolving the admin once here is what lets a builder start with its first row instead of the
 * same four lines of lookups and null checks.
 */
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

}  // namespace AdminSystem::Admin::Menu
