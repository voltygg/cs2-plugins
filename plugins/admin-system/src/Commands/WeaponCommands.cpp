#include "../Core/App.hpp"
#include "../Core/Permissions.hpp"
#include "../Weapons/WeaponCatalog.hpp"
#include "Commands.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Runtime.hpp>
#include <string>

namespace AdminSystem::Commands
{

using namespace VoltMod::Commands;
using AdminSystem::Weapons::WeaponMatch;
using VoltMod::Tokens;

void RegisterWeaponCommands(VoltMod::CommandManager& commands, App& app)
{
    commands.Register({
        .Name = "give",
        .Description = "Give a configured weapon to a player.",
        .Permission = Flag(Permission::Weapon),
        .Args = {Target(), Word()},
        .Handler =
            [&app](CommandContext& c) {
                auto lookup = Weapons::FindWeapon(app.Config.GetWeaponMenu(), c.Word);
                Tokens tokens{{"weapon", c.Word}};

                if (lookup.Result == WeaponMatch::Ambiguous)
                {
                    tokens["count"] = std::to_string(lookup.Count);
                    return c.Fail("cmd.weaponAmbiguous", tokens);
                }
                if (lookup.Result == WeaponMatch::None)
                    return c.Fail("cmd.weaponNoMatch", tokens);

                const auto& weapon = app.Config.GetWeaponMenu()[lookup.Index];
                auto ctx = app.Actions.Resolve(c.CallerSlot(), c.Target().GetSlot(), Flag(Permission::Weapon));
                if (!ctx.Valid())
                    return CommandResult::Silent();
                if (!ctx.TargetCtrl.IsAlive())
                    return c.Fail("cmd.weaponTargetDead", {{"name", c.Target().GetName()}});

                if (!app.Runtime.Items.Give(ctx.TargetCtrl, weapon.Item.c_str()))
                    return c.Fail("cmd.weaponGiveFailed", {{"weapon", weapon.Label()}});

                app.Chat.BroadcastAction("broadcast.gaveWeapon", c.Caller->GetName(), c.Target().GetName());
                return c.Ok("cmd.weaponGiven", {{"weapon", weapon.Label()}, {"name", c.Target().GetName()}});
            },
    });

    commands.Register({
        .Name = "strip",
        .Description = "Remove every weapon a player is carrying.",
        .Permission = Flag(Permission::Weapon),
        .Args = {Target()},
        .Handler =
            [&app](CommandContext& c) {
                auto ctx = app.Actions.Resolve(c.CallerSlot(), c.Target().GetSlot(), Flag(Permission::Weapon));
                if (!ctx.Valid())
                    return CommandResult::Silent();
                if (!ctx.TargetCtrl.IsAlive())
                    return c.Fail("cmd.weaponTargetDead", {{"name", c.Target().GetName()}});

                if (!app.Runtime.Items.StripWeapons(ctx.TargetCtrl))
                    return c.Fail("cmd.weaponStripFailed");

                app.Chat.BroadcastAction("broadcast.stripped", c.Caller->GetName(), c.Target().GetName());
                return CommandResult::Silent();
            },
    });
}

}  // namespace AdminSystem::Commands
