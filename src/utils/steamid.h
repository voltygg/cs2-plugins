#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace utils
{

/**
 * @brief SteamID utility class for converting between formats
 *
 * Supports conversion between:
 * - SteamID64 (int64_t, e.g., 76561198012345678)
 * - SteamID3 (string, e.g., [U:1:52079950])
 * - SteamID (string, e.g., STEAM_0:0:26039975)
 */
class SteamID
{
public:
    /**
     * @brief Convert SteamID64 to SteamID3 format
     * @param steamid64 The 64-bit Steam ID
     * @return SteamID3 string (e.g., "[U:1:52079950]")
     */
    static std::string ToSteamID3(int64_t steamid64);

    /**
     * @brief Convert SteamID64 to legacy SteamID format
     * @param steamid64 The 64-bit Steam ID
     * @return SteamID string (e.g., "STEAM_0:0:26039975")
     */
    static std::string ToSteamID(int64_t steamid64);

    /**
     * @brief Convert SteamID3 to SteamID64
     * @param steamid3 SteamID3 string (e.g., "[U:1:52079950]")
     * @return SteamID64 or std::nullopt if invalid
     */
    static std::optional<int64_t> FromSteamID3(const std::string& steamid3);

    /**
     * @brief Convert legacy SteamID to SteamID64
     * @param steamid Legacy SteamID string (e.g., "STEAM_0:0:26039975")
     * @return SteamID64 or std::nullopt if invalid
     */
    static std::optional<int64_t> FromSteamID(const std::string& steamid);

    /**
     * @brief Check if a SteamID64 is valid
     * @param steamid64 The 64-bit Steam ID
     * @return true if valid, false otherwise
     */
    static bool IsValid(int64_t steamid64);

    /**
     * @brief Extract account ID from SteamID64
     * @param steamid64 The 64-bit Steam ID
     * @return Account ID (32-bit)
     */
    static uint32_t GetAccountID(int64_t steamid64);

private:
    static constexpr int64_t STEAMID64_BASE = 76561197960265728LL;
};

}  // namespace utils
