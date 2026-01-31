#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace utils {

/**
 * @brief Time utility class for formatting and parsing durations
 */
class Time
{
public:
    using TimePoint = std::chrono::system_clock::time_point;
    using Duration = std::chrono::seconds;

    /**
     * @brief Get current Unix timestamp in seconds
     * @return Current timestamp
     */
    static int64_t Now();

    /**
     * @brief Parse duration string (e.g., "30m", "1h", "7d", "0" for permanent)
     * @param duration String representation (format: <number><unit>)
     *                 Units: s (seconds), m (minutes), h (hours), d (days), w (weeks)
     *                 Special: "0" or "perm" for permanent (returns 0)
     * @return Duration in seconds, or 0 for permanent
     */
    static int64_t ParseDuration(const std::string& duration);

    /**
     * @brief Format duration into human-readable string
     * @param seconds Duration in seconds (0 = permanent)
     * @return Formatted string (e.g., "30 minutes", "2 hours", "Permanent")
     */
    static std::string FormatDuration(int64_t seconds);

    /**
     * @brief Format timestamp into readable date/time
     * @param timestamp Unix timestamp in seconds
     * @return Formatted string (e.g., "2025-01-31 12:30:45")
     */
    static std::string FormatTimestamp(int64_t timestamp);

    /**
     * @brief Check if a punishment has expired
     * @param expiresAt Expiration timestamp (0 = permanent)
     * @return true if expired, false if still active or permanent
     */
    static bool IsExpired(int64_t expiresAt);

    /**
     * @brief Get expiration timestamp from duration
     * @param durationSeconds Duration in seconds (0 = permanent)
     * @return Expiration timestamp (0 if permanent)
     */
    static int64_t GetExpirationTime(int64_t durationSeconds);

private:
    static constexpr int64_t SECONDS_PER_MINUTE = 60;
    static constexpr int64_t SECONDS_PER_HOUR = 3600;
    static constexpr int64_t SECONDS_PER_DAY = 86400;
    static constexpr int64_t SECONDS_PER_WEEK = 604800;
};

} // namespace utils
