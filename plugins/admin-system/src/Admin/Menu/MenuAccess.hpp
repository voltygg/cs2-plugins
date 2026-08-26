#pragma once

// Re-checking a menu row per click is the same question the row asked when it was rendered, and
// both go to VoltMod::Policy::Authorize. These two spell it for the menu files so no plugin file
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

/** Whether @p adminSlot still holds @p permission and may act on @p targetSlot. */
inline bool MayUseOn(App& app, int adminSlot, int targetSlot, Permission permission)
{
    auto& players = app.Runtime.Players;
    return app.Runtime.Policy.Authorize(players.RefFor(adminSlot), players.RefFor(targetSlot), Flag(permission))
        .has_value();
}

}  // namespace AdminSystem::Admin::Menu
