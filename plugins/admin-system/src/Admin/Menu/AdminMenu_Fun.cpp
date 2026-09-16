#include "Admin/Menu/AdminMenu_Fun.hpp"

#include "Admin/Menu/MenuAccess.hpp"
#include "Core/App.hpp"
#include "Core/ChatService.hpp"
#include "Core/Permissions.hpp"
#include "Fun/FunMode.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <string>
#include <string_view>

namespace AdminSystem::Admin::Menu
{

using VoltMod::ButtonRow;
using VoltMod::MenuBuilder;
using VoltMod::ToggleRow;

std::shared_ptr<VoltMod::Menu> BuildFunMenu(AdminSystem::App& app, int adminSlot)
{
    auto& translations = app.Runtime.Translations;

    if (!app.Runtime.Players.Get(adminSlot))
        return nullptr;

    const VoltMod::EnabledCondition allowed = Allows(app, Permission::FunMode);

    MenuBuilder builder(translations.Get("category.fun", adminSlot));

    for (const auto& info : Fun::Toggles)
    {
        builder.Add(ToggleRow{
            .Label = translations.Get(std::string(info.NameKey), adminSlot),
            .Get = [&app, id = info.Id](int) { return app.FunMode.IsOn(id); },
            .Flip =
                [&app, id = info.Id, onKey = std::string(info.OnKey), offKey = std::string(info.OffKey)](int slot) {
                    app.Chat.BroadcastAction(app.FunMode.Flip(id) ? onKey : offKey, ActorName(app, slot),
                                             std::string_view{});
                },
            .Enabled = allowed});
    }

    builder.Add(ButtonRow{.Label = translations.Get("fun.clearAll", adminSlot),
                          .Activate =
                              [&app](int slot) {
                                  app.FunMode.ClearAll();
                                  app.Chat.BroadcastAction("broadcast.funCleared", ActorName(app, slot),
                                                           std::string_view{});
                              },
                          .Enabled = allowed});

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
