#include "Core/AdminMenuSection.hpp"

#include "App.hpp"

#include <VoltMod/Api.hpp>

namespace AdminSystem::Core
{

void AdminMenuSection::Publish()
{
    _app.Runtime.Exchange.Publish<Contracts::IMenuSection>(this, "admin");
}

void AdminMenuSection::Unpublish()
{
    _app.Runtime.Exchange.Unpublish<Contracts::IMenuSection>("admin");
}

bool AdminMenuSection::IsVisibleTo(int slot)
{
    const VoltMod::Player* player = _app.Runtime.Players.Get(slot);
    return player && _app.Admins.IsAdmin(player->SteamId());
}

bool AdminMenuSection::Open(int slot)
{
    return IsVisibleTo(slot) && _app.OpenAdminMenu(slot);
}

}  // namespace AdminSystem::Core