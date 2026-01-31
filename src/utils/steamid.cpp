#include "steamid.h"
#include <format>
#include <regex>

namespace utils {

std::string SteamID::ToSteamID3(int64_t steamid64)
{
    uint32_t accountId = GetAccountID(steamid64);
    return std::format("[U:1:{}]", accountId);
}

std::string SteamID::ToSteamID(int64_t steamid64)
{
    uint32_t accountId = GetAccountID(steamid64);
    uint32_t authServer = accountId % 2;
    uint32_t accountNum = accountId / 2;
    return std::format("STEAM_0:{}:{}", authServer, accountNum);
}

std::optional<int64_t> SteamID::FromSteamID3(const std::string& steamid3)
{
    // Pattern: [U:1:XXXXXXXXXX]
    std::regex pattern(R"(\[U:1:(\d+)\])");
    std::smatch matches;

    if (std::regex_match(steamid3, matches, pattern))
    {
        try
        {
            uint32_t accountId = std::stoul(matches[1].str());
            int64_t steamid64 = STEAMID64_BASE + accountId;
            return steamid64;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

std::optional<int64_t> SteamID::FromSteamID(const std::string& steamid)
{
    // Pattern: STEAM_X:Y:ZZZZZZZZ
    std::regex pattern(R"(STEAM_[0-5]:([01]):(\d+))");
    std::smatch matches;

    if (std::regex_match(steamid, matches, pattern))
    {
        try
        {
            uint32_t authServer = std::stoul(matches[1].str());
            uint32_t accountNum = std::stoul(matches[2].str());
            uint32_t accountId = accountNum * 2 + authServer;
            int64_t steamid64 = STEAMID64_BASE + accountId;
            return steamid64;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

bool SteamID::IsValid(int64_t steamid64)
{
    // Valid SteamID64 range for individual accounts
    // Starts at 76561197960265728 (STEAMID64_BASE)
    // Max is around 76561202255233023 (as of 2025)
    return steamid64 >= STEAMID64_BASE && steamid64 < (STEAMID64_BASE + 0x100000000LL);
}

uint32_t SteamID::GetAccountID(int64_t steamid64)
{
    return static_cast<uint32_t>(steamid64 - STEAMID64_BASE);
}

} // namespace utils
