#pragma once

#include "Admin/Menu/MenuCatalog.hpp"
#include "Core/App.hpp"
#include "Core/Permissions.hpp"

#include <VoltMod/Menu/Menu.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Runtime.hpp>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace AdminSystem::Admin::Menu
{

/** Whether @p slot still holds @p permission. A permission may have been revoked (e.g. by
 *  !admin_reload) while the menu was open. */
inline bool MayUse(App& app, int slot, std::string_view permission)
{
    auto& players = app.Runtime.Players;
    return app.Runtime.Policy.Authorize(players.RefFor(slot), std::nullopt, permission).has_value();
}

/** @ref MayUse as a row's condition: asked on every redraw, and it refuses the press itself, so
 *  no handler repeats the check. */
inline VoltMod::EnabledCondition Allows(App& app, std::string_view permission)
{
    return VoltMod::EnabledCondition([&app, permission](int slot) { return MayUse(app, slot, permission); });
}

/** Flow validator that re-checks @p permission on @p slot, the one player the flow runs for: a
 *  permission may have been revoked (e.g. by !admin_reload) while the menu was open. */
inline auto RequirePermission(App& app, std::string_view permission, int slot)
{
    return [&app, permission, slot](const auto&) -> std::optional<std::string> {
        if (!MayUse(app, slot, permission))
            return "punish.notAllowed";
        return std::nullopt;
    };
}

/**
 * @brief Whether @p adminSlot may see @p row at all.
 *
 * Permission only, and the answer decides whether the row is built. A row the admin holds the
 * permission for but cannot use right now - a dead target, a vote that is not running - stays
 * visible and greys itself instead, so rows do not appear and vanish under the cursor.
 */
[[nodiscard]] bool Visible(App& app, int adminSlot, const RowSpec& row);

/** Whether any of @p rows is visible to @p adminSlot. Walks permission strings; it never builds a
 *  child menu, so asking about a whole tab costs one pass over a constexpr table. */
[[nodiscard]] bool AnyVisible(App& app, int adminSlot, std::span<const RowSpec> rows);

/**
 * @brief @p item greyed out, showing @p reason, whenever @p usable says no.
 *
 * `EnabledCondition` only flips a row grey and no row spec exposes `MenuRow::Value`, so the
 * reason is folded into `Describe` here. Activation, stepping and commit are wrapped too, or a
 * greyed preset row would still change its value.
 */
[[nodiscard]] VoltMod::MenuItem DisableUnless(VoltMod::MenuItem item, std::function<bool(int slot)> usable,
                                              std::string reason);

/** @ref DisableUnless for an action whose descriptor refuses a dead target. The dispatcher skips
 *  those silently, so without this the row looks live and does nothing. */
[[nodiscard]] VoltMod::MenuItem WhileAlive(App& app, int adminSlot, VoltMod::PlayerRef target, VoltMod::MenuItem item);

/** @ref DisableUnless for a row targeting @p target: an admin who outranks the viewer greys the
 *  row once, in the player list, instead of every row of a card that opens dead. */
[[nodiscard]] VoltMod::MenuItem WhileTargetable(App& app, VoltMod::PlayerRef admin, VoltMod::PlayerRef target,
                                                VoltMod::MenuItem item);

/**
 * @brief Warn about any row whose catalog permission differs from the one its descriptor runs on.
 *
 * Declared here rather than beside the tables because it needs @ref App, and MenuCatalog.hpp stays
 * free of the game SDK so the catalog test can link it. Run once at load: this is the drift that
 * twice left a tab greyed out for the very admins it was meant for.
 */
void VerifyCatalog(App& app);

/** Appends what @p make builds, but only when @p row is visible to @p adminSlot. */
template <class Make>
void AddIfVisible(App& app, int adminSlot, VoltMod::MenuBuilder& builder, const RowSpec& row, Make make)
{
    if (Visible(app, adminSlot, row))
        builder.Add(make());
}

}  // namespace AdminSystem::Admin::Menu
