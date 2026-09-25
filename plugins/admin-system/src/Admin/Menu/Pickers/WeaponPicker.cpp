#include "Admin/Menu/Pickers/WeaponPicker.hpp"

#include "App.hpp"
#include "Core/ChatService.hpp"
#include "Weapons/WeaponActions.hpp"
#include "Weapons/WeaponCatalog.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Random.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <memory>
#include <string_view>

namespace AdminSystem::Admin::Menu
{

using VoltMod::MenuBuilder;

/** Tell the admin why a weapon action did nothing. The menu is the only way to reach these, so
 *  a silent no-op would leave them guessing whether the click registered. */
static void ReportWeaponOutcome(AdminSystem::App& app, int adminSlot, Weapons::WeaponActionResult result,
                                std::string_view failKey)
{
    auto& translations = app.Runtime.Translations;
    switch (result)
    {
    case Weapons::WeaponActionResult::TargetDead:
        app.Chat.Reply(adminSlot, translations.Get("cmd.weaponTargetDead", adminSlot));
        break;
    case Weapons::WeaponActionResult::EngineRefused:
        app.Chat.Reply(adminSlot, translations.Get(failKey, adminSlot));
        break;
    case Weapons::WeaponActionResult::NotAllowed:  // the dispatcher's policy already replied
    case Weapons::WeaponActionResult::Ok:
        break;
    }
}

std::shared_ptr<VoltMod::Menu> BuildWeaponPicker(const MenuContext& ctx, VoltMod::PlayerRef target)
{
    App& app = ctx.Plugin;
    const VoltMod::PlayerRef admin = ctx.Admin;

    const auto title = ctx.CardTitle("action.giveWeapon", target);
    if (!title)
    {
        return nullptr;
    }

    MenuBuilder builder(*title);

    const auto& menu = app.Settings.GetWeaponMenu();
    builder.EmptyText(ctx.Translate("action.noWeapons"));
    for (const auto& weapon : menu)
    {
        builder.Button(weapon.Label(), [&app, admin, target, item = weapon.Item](int slot) {
            ReportWeaponOutcome(app, slot, Weapons::GiveWeapon(app, admin, target, item), "cmd.weaponGiveFailed");
        });
    }

    if (!menu.empty())
    {
        builder.Button(ctx.Translate("action.giveRandomWeapon"), [&app, admin, target](int slot) {
            const auto& weapons = app.Settings.GetWeaponMenu();
            if (weapons.empty())
            {
                return;
            }
            ReportWeaponOutcome(
                app, slot, Weapons::GiveWeapon(app, admin, target, weapons[VoltMod::RandomIndex(weapons.size())].Item),
                "cmd.weaponGiveFailed");
        });
    }

    builder.Button(ctx.Translate("action.strip"), [&app, admin, target](int slot) {
        ReportWeaponOutcome(app, slot, Weapons::StripWeapons(app, admin, target), "cmd.weaponStripFailed");
    });

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
