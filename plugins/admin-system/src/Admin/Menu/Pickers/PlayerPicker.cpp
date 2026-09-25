#include "Admin/Menu/Pickers/PlayerPicker.hpp"

#include "Admin/Menu/MenuAccess.hpp"
#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Menu/MenuPresets.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <utility>

namespace AdminSystem::Admin::Menu
{

std::shared_ptr<VoltMod::Menu> BuildPlayerPicker(AdminSystem::App& app, int adminSlot, VoltMod::PlayerPicker spec)
{
    spec.EmptyLabel = app.Runtime.Translations.Get("common.noPlayers", adminSlot);
    return ::VoltMod::BuildPlayerPicker(app.Runtime.Players, std::move(spec));
}

void AppendTargetRows(const MenuContext& ctx, VoltMod::MenuBuilder& builder,
                      std::function<std::shared_ptr<VoltMod::Menu>(VoltMod::PlayerRef target)> open)
{
    App& app = ctx.Plugin;

    auto connected = app.Runtime.Players.All();
    for (auto* player : connected)
    {
        // Resolve the original player, not a later occupant of the slot.
        const VoltMod::PlayerRef target = player->Ref();

        VoltMod::MenuItem row = VoltMod::SubmenuRow{
            .Label = player->Name(), .Build = [open, target](int) {
                return open(target);
            }}.ToItem();
        builder.Add(std::move(row));
    }

    if (connected.empty())
    {
        builder.Text(ctx.Translate("common.noPlayers"));
    }
}

}  // namespace AdminSystem::Admin::Menu
