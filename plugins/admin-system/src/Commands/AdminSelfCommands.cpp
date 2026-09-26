#include "Admin/Effects/Descriptors.hpp"
#include "App.hpp"
#include "Commands/Commands.hpp"
#include "Core/Permissions.hpp"

#include <VoltMod/Api.hpp>

using VoltMod::Caller;
using VoltMod::Reply;
using VoltMod::Result;

namespace AdminSystem::Commands
{

void RegisterAdminSelfCommands(VoltMod::CommandManager& commands, App& app)
{
    commands.Add("hide")
        .Describe("Toggle stealth-spectator mode on yourself.")
        .Permission(Permission::Hide)
        .Run([&app](Caller c) {
            const auto self = c.Player->Ref();
            app.PlayerEffects.Toggle(self, self, app.EffectDescriptors.Hide);
        });
}

}  // namespace AdminSystem::Commands
