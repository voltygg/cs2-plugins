#pragma once

// Re-checking a menu row per click is the same question the row asked when it was rendered, and
// both go to VoltMod::Policy::Authorize. This spells it for the menu files so no plugin file
// repeats the permission/immunity rules.

#include "../../Core/App.hpp"
#include "../../Core/Permissions.hpp"

#include <VoltMod/Runtime.hpp>
#include <optional>

namespace AdminSystem::Admin::Menu
{

/** Whether @p slot still holds @p permission. A flag may have been revoked (e.g. by
 *  !admin_reload) while the menu was open. */
inline bool MayUse(App& app, int slot, Permission permission)
{
    auto& players = app.Runtime.Players;
    return app.Runtime.Policy.Authorize(players.RefFor(slot), std::nullopt, Flag(permission)).has_value();
}

/** Flow validator that re-checks @p permission on the presser: a flag may have been revoked
 *  (e.g. by !admin_reload) while the menu was open. */
inline auto RequirePermission(App& app, Permission permission)
{
    return [&app, permission](int slot, const auto&) -> std::optional<std::string> {
        if (!MayUse(app, slot, permission))
            return "punish.notAllowed";
        return std::nullopt;
    };
}

}  // namespace AdminSystem::Admin::Menu
