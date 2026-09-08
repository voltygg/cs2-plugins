#pragma once

#include "../../Core/App.hpp"
#include "../../Core/Permissions.hpp"

#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Runtime.hpp>
#include <optional>
#include <string>

namespace AdminSystem::Admin::Menu
{

/** Whether @p slot still holds @p permission. A flag may have been revoked (e.g. by
 *  !admin_reload) while the menu was open. */
inline bool MayUse(App& app, int slot, Permission permission)
{
    auto& players = app.Runtime.Players;
    return app.Runtime.Policy.Authorize(players.RefFor(slot), std::nullopt, Flag(permission)).has_value();
}

/** @ref MayUse as a row's condition: asked on every redraw, and it refuses the press itself, so
 *  no handler repeats the check. */
inline VoltMod::EnabledCondition Allows(App& app, Permission permission)
{
    return VoltMod::EnabledCondition([&app, permission](int slot) { return MayUse(app, slot, permission); });
}

/** Flow validator that re-checks @p permission on @p slot, the one player the flow runs for: a
 *  flag may have been revoked (e.g. by !admin_reload) while the menu was open. */
inline auto RequirePermission(App& app, Permission permission, int slot)
{
    return [&app, permission, slot](const auto&) -> std::optional<std::string> {
        if (!MayUse(app, slot, permission))
            return "punish.notAllowed";
        return std::nullopt;
    };
}

}  // namespace AdminSystem::Admin::Menu
