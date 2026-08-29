#include "PlayerPicker.hpp"

#include "../../Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/MenuPresets.hpp>
#include <VoltMod/Runtime.hpp>
#include <utility>

namespace AdminSystem::Admin::Menu
{

// The defaults every picker in this plugin carries, whichever entry point draws it.
static void ApplyDefaults(AdminSystem::App& app, int adminSlot, VoltMod::PlayerPicker& spec)
{
    spec.EmptyLabel = app.Runtime.Translations.Get("common.noPlayers", adminSlot);
}

std::shared_ptr<VoltMod::Menu> BuildPlayerPicker(AdminSystem::App& app, int adminSlot, VoltMod::PlayerPicker spec)
{
    ApplyDefaults(app, adminSlot, spec);
    return ::VoltMod::BuildPlayerPicker(app.Runtime.Players, std::move(spec));
}

void AppendPlayerRows(AdminSystem::App& app, int adminSlot, VoltMod::MenuBuilder& builder, VoltMod::PlayerPicker spec)
{
    ApplyDefaults(app, adminSlot, spec);
    ::VoltMod::AppendPlayerRows(builder, app.Runtime.Players, spec);
}

}  // namespace AdminSystem::Admin::Menu
