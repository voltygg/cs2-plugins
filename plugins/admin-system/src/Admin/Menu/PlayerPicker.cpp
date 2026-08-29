#include "PlayerPicker.hpp"

#include "../../Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/MenuPresets.hpp>
#include <VoltMod/Runtime.hpp>
#include <utility>

namespace AdminSystem::Admin::Menu
{

std::shared_ptr<VoltMod::Menu> BuildPlayerPicker(AdminSystem::App& app, int adminSlot, VoltMod::PlayerPicker spec)
{
    spec.EmptyLabel = app.Runtime.Translations.Get("common.noPlayers", adminSlot);
    return ::VoltMod::BuildPlayerPicker(app.Runtime.Players, std::move(spec));
}

}  // namespace AdminSystem::Admin::Menu
