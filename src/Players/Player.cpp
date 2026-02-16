#include "Player.hpp"
#include "../Utils/TimeUtils.hpp"

namespace AdminSystem::Players {

Player::Player(int slot, int64_t steamId, const std::string& name, const std::string& ipAddress)
    : _slot(slot), _steamId(steamId), _name(name), _ipAddress(ipAddress),
      _connectTime(Utils::TimeUtils::Now())
{
}

int64_t Player::GetPlaytime() const
{
    return Utils::TimeUtils::Now() - _connectTime;
}

} // namespace AdminSystem::Players
