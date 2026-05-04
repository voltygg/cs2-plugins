#pragma once

#include "../Entities/VoiceMute.hpp"

#include <optional>
#include <pqxx/pqxx>
#include <vector>

namespace AdminSystem::Database
{

/** Repository for voice-mute records — lookup, creation, removal, expiration, and history. */
class VoiceMuteRepository
{
public:
    std::optional<VoiceMute> FindActiveBySteamId(int64_t steamId);
    std::vector<VoiceMute> FindAllActive();
    bool Create(VoiceMute& mute);
    bool Remove(int64_t muteId, int64_t removedBy, const std::string& reason);
    int ExpireOldVoiceMutes();
    std::vector<VoiceMute> GetHistory(int64_t steamId);

private:
    VoiceMute ParseRow(const pqxx::row& row);
};

}  // namespace AdminSystem::Database
