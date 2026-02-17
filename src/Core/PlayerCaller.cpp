#include "PlayerCaller.hpp"

#include "../Players/Player.hpp"

namespace AdminSystem::Core
{

int64_t PlayerCaller::GetSteamID() const
{
    return _player ? _player->GetSteamID() : 0;
}

std::string PlayerCaller::GetName() const
{
    return _player ? _player->GetName() : "";
}

bool PlayerCaller::IsServerConsole() const
{
    return false;
}

}  // namespace AdminSystem::Core
