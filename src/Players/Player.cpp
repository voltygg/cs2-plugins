#include "Player.hpp"

#include "../../vendor/cs2-kit/src/Utils/TimeUtils.hpp"

namespace AdminSystem::Players
{

using namespace CS2Kit::Utils;

Player::Player(int slot, int64_t steamId, const std::string& name, const std::string& ipAddress)
    : _slot(slot), _steamId(steamId), _name(name), _ipAddress(ipAddress), _connectTime(TimeUtils::Now())
{}

int64_t Player::GetPlaytime() const
{
    return TimeUtils::Now() - _connectTime;
}

}  // namespace AdminSystem::Players
