#include "AdminMenu_Fun.hpp"

#include "../../Core/App.hpp"
#include "../../Core/ChatService.hpp"
#include "../../Core/Permissions.hpp"
#include "../../Fun/FunMode.hpp"
#include "MenuAccess.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <string>

namespace AdminSystem::Admin::Menu
{

using AdminSystem::Fun::Toggle;
using VoltMod::MenuBuilder;

std::shared_ptr<VoltMod::MenuView> BuildFunMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;

    auto* admin = app.Runtime.Players.Get(adminSlot);
    if (!admin)
        return nullptr;

    bool allowed = app.Access.HasPermission(admin->SteamId(), Permission::FunMode);

    MenuBuilder builder(tr.Get("category.fun", adminSlot));

    for (const auto& info : Fun::Toggles)
    {
        builder.Toggle(
            tr.Get(std::string(info.NameKey), adminSlot), tr.Get("effectState.on", adminSlot),
            tr.Get("effectState.off", adminSlot), [&app, id = info.Id](int) { return app.FunMode.IsOn(id); },
            [&app, id = info.Id, onKey = std::string(info.OnKey), offKey = std::string(info.OffKey)](int slot) {
                // Re-check per click: the menu may have been open across an !admin_reload.
                if (!MayUse(app, slot, Permission::FunMode))
                    return;
                bool on = app.FunMode.Flip(id);
                app.Chat.BroadcastKey(on ? onKey : offKey);
            },
            allowed);
    }

    builder.Button(
        tr.Get("fun.clearAll", adminSlot),
        [&app](int slot) {
            if (!MayUse(app, slot, Permission::FunMode))
                return;
            app.FunMode.ClearAll();
            app.Chat.BroadcastKey("broadcast.funCleared");
        },
        allowed);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
