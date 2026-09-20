#include "Admin/Menu/MenuCatalog.hpp"

#include "Admin/Actions/Descriptors.hpp"
#include "Admin/Effects/Descriptors.hpp"
#include "Admin/Menu/MenuAccess.hpp"
#include "Core/App.hpp"

#include <VoltMod/Core/Log.hpp>
#include <span>

namespace AdminSystem::Admin::Menu
{

/** The permission the descriptor behind @p id dispatches on, or empty for a row that has none.
 *  A constexpr table cannot reach these, which is why the catalog repeats them. Every row is
 *  named and there is no `default`, so a new one has to be placed here before it compiles. */
static std::string_view DispatchPermission(App& app, RowId id)
{
    const auto& actions = app.ActionDescriptors;
    const auto& effects = app.EffectDescriptors;

    switch (id)
    {
    case RowId::Kill:
        return Actions::Kill.Permission;
    case RowId::Bring:
        return Actions::Bring.Permission;
    case RowId::Goto:
        return Actions::Goto.Permission;
    case RowId::Freeze:
        return Actions::Freeze.Permission;
    case RowId::Noclip:
        return Actions::Noclip.Permission;
    case RowId::Bury:
        return Actions::Bury.Permission;
    case RowId::Unbury:
        return Actions::Unbury.Permission;
    case RowId::ChangeTeam:
        return Actions::ChangeTeam.Permission;
    case RowId::Speed:
        return Actions::SetSpeed.Permission;
    case RowId::Slap:
        return actions.Slap.Permission;
    case RowId::Health:
        return Actions::SetHealth.Permission;
    case RowId::Armor:
        return Actions::SetArmor.Permission;
    case RowId::Godmode:
        return Actions::Godmode.Permission;
    case RowId::Ghost:
        return effects.Ghost.Permission;
    case RowId::Disco:
        return effects.Disco.Permission;
    case RowId::Wallhack:
        return effects.Wallhack.Permission;
    case RowId::Model:
        return effects.Model.Permission;
    case RowId::Bhop:
        return effects.Bhop.Permission;
    case RowId::Drunk:
        return effects.Drunk.Permission;
    case RowId::Smite:
        return actions.Smite.Permission;
    case RowId::Size:
        return actions.SetSize.Permission;
    case RowId::Hide:
        return effects.Hide.Permission;

    // No descriptor to compare against: punishments dispatch through PunishTypes, which
    // MenuCatalogTests pins to the catalog, and the rest check the catalog's permission directly.
    case RowId::LiftList:
    case RowId::PunishPlayers:
    case RowId::LiftBans:
    case RowId::LiftMutes:
    case RowId::PunishKick:
    case RowId::PunishBan:
    case RowId::PunishVoiceMute:
    case RowId::PunishTextMute:
    case RowId::PunishWarn:
    case RowId::CheatCheck:
    case RowId::Swap:
    case RowId::Weapons:
    case RowId::ChangeMap:
    case RowId::SetNextMap:
    case RowId::VoteMap:
    case RowId::CancelVote:
    case RowId::ChatPrefix:
    case RowId::NameColor:
    case RowId::MessageColor:
        return {};
    }
    return {};
}

static void CheckRows(App& app, std::span<const RowSpec> rows)
{
    for (const RowSpec& row : rows)
    {
        const std::string_view dispatch = DispatchPermission(app, row.Id);
        if (!dispatch.empty() && dispatch != row.Permission)
        {
            VoltMod::Log::Warn("Admin menu row '{}' is shown on '{}' but runs on '{}'; one of the two is wrong.",
                               row.LabelKey, row.Permission, dispatch);
        }
        CheckRows(app, row.Children);
    }
}

void VerifyCatalog(App& app)
{
    for (const TabSpec& tab : Tabs)
        CheckRows(app, tab.Rows);
}

}  // namespace AdminSystem::Admin::Menu
