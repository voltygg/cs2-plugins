#pragma once

#include "../Entities/TextMute.hpp"

#include <optional>
#include <pqxx/pqxx>
#include <vector>

namespace AdminSystem::Database
{

/** Repository for text-mute records — lookup, creation, removal, expiration, and history. */
class TextMuteRepository
{
public:
    std::optional<TextMute> FindActiveBySteamId(int64_t steamId);
    std::vector<TextMute> FindAllActive();
    bool Create(TextMute& mute);
    bool Remove(int64_t muteId, int64_t removedBy, const std::string& reason);
    int ExpireOldTextMutes();
    std::vector<TextMute> GetHistory(int64_t steamId);

private:
    TextMute ParseRow(const pqxx::row& row);
};

}  // namespace AdminSystem::Database
