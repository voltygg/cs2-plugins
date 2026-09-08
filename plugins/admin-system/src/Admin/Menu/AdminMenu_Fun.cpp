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

using VoltMod::ButtonRow;
using VoltMod::MenuBuilder;
using VoltMod::ToggleRow;

std::shared_ptr<VoltMod::Menu> BuildFunMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;

    if (!app.Runtime.Players.Get(adminSlot))
        return nullptr;

    const VoltMod::EnabledCondition allowed = Allows(app, Permission::FunMode);

    MenuBuilder builder(tr.Get("category.fun", adminSlot));

    for (const auto& info : Fun::Toggles)
    {
        builder.Add(
            ToggleRow{.Label = tr.Get(std::string(info.NameKey), adminSlot),
                      .Get = [&app, id = info.Id](int) { return app.FunMode.IsOn(id); },
                      .Flip = [&app, id = info.Id, onKey = std::string(info.OnKey), offKey = std::string(info.OffKey)](
                                  int) { app.Chat.BroadcastKey(app.FunMode.Flip(id) ? onKey : offKey); },
                      .Enabled = allowed});
    }

    builder.Add(ButtonRow{.Label = tr.Get("fun.clearAll", adminSlot),
                          .Activate =
                              [&app](int) {
                                  app.FunMode.ClearAll();
                                  app.Chat.BroadcastKey("broadcast.funCleared");
                              },
                          .Enabled = allowed});

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
