#include "player.h"

#include "../utils/time.h"

namespace player
{

Player::Player(int slot, int64_t steam_id, const std::string& name, const std::string& ip_address)
    : m_slot(slot),
      m_steamID(steam_id),
      m_name(name),
      m_ipAddress(ip_address),
      m_connectTime(utils::Time::Now()),
      m_isAdmin(false),
      m_isMuted(false),
      m_isGagged(false)
{
}

int64_t Player::GetPlaytime() const
{
    return utils::Time::Now() - m_connectTime;
}

}  // namespace player
