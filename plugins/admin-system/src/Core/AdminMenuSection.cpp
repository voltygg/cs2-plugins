#include "Core/AdminMenuSection.hpp"

#include <VoltMod/Api.hpp>

namespace AdminSystem::Core
{

void AdminMenuSection::Publish()
{
    _rt.Exchange.PublishNamed(Contracts::MenuSectionName("admin"),
                              static_cast<void*>(static_cast<Contracts::IMenuSection*>(this)));
}

void AdminMenuSection::Unpublish()
{
    _rt.Exchange.UnpublishNamed(Contracts::MenuSectionName("admin"));
}

bool AdminMenuSection::IsVisibleTo(int slot)
{
    const VoltMod::Player* player = _rt.Players.Get(slot);
    return player && _admins.IsAdmin(player->SteamId());
}

bool AdminMenuSection::Open(int slot)
{
    return IsVisibleTo(slot) && _open(slot);
}

}  // namespace AdminSystem::Core
