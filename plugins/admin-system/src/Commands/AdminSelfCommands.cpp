#include "../Admin/Effects/Descriptors.hpp"
#include "../Core/App.hpp"
#include "../Core/Permissions.hpp"
#include "Commands.hpp"

#include <VoltMod/Api.hpp>

using VoltMod::Caller;
using VoltMod::Reply;
using VoltMod::Result;

namespace AdminSystem::Commands
{

void RegisterAdminSelfCommands(VoltMod::CommandManager& commands, App& app, Subs& subs)
{
    subs.push_back(commands.Add("hide")
                       .Describe("Toggle stealth-spectator mode on yourself.")
                       .Permission(Flag(Permission::Hide))
                       .Run([&app](Caller c) -> Result<Reply> {
                           const auto self = app.Runtime.Players.RefFor(c.Slot);
                           app.PlayerEffects.Toggle(self, self, app.EffectDescriptors.Hide);
                           return Reply::Silent();
                       }));
}

}  // namespace AdminSystem::Commands
