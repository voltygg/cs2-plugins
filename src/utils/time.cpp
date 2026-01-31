#include "time.h"
#include <format>
#include <regex>
#include <cctype>
#include <algorithm>

namespace utils {

int64_t Time::Now()
{
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

int64_t Time::ParseDuration(const std::string& duration)
{
    // Handle empty or "0" or "perm" as permanent
    std::string lower = duration;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower.empty() || lower == "0" || lower == "perm" || lower == "permanent")
    {
        return 0;
    }

    // Pattern: <number><unit>
    std::regex pattern(R"((\d+)([smhdw]))");
    std::smatch matches;

    if (std::regex_match(lower, matches, pattern))
    {
        try
        {
            int64_t value = std::stoll(matches[1].str());
            char unit = matches[2].str()[0];

            switch (unit)
            {
            case 's':
                return value;
            case 'm':
                return value * SECONDS_PER_MINUTE;
            case 'h':
                return value * SECONDS_PER_HOUR;
            case 'd':
                return value * SECONDS_PER_DAY;
            case 'w':
                return value * SECONDS_PER_WEEK;
            default:
                return 0;
            }
        }
        catch (...)
        {
            return 0;
        }
    }

    // Try parsing as raw seconds
    try
    {
        return std::stoll(duration);
    }
    catch (...)
    {
        return 0;
    }
}

std::string Time::FormatDuration(int64_t seconds)
{
    if (seconds == 0)
    {
        return "Permanent";
    }

    // Format largest applicable unit
    if (seconds % SECONDS_PER_WEEK == 0 && seconds >= SECONDS_PER_WEEK)
    {
        int64_t weeks = seconds / SECONDS_PER_WEEK;
        return std::format("{} week{}", weeks, weeks > 1 ? "s" : "");
    }
    else if (seconds % SECONDS_PER_DAY == 0 && seconds >= SECONDS_PER_DAY)
    {
        int64_t days = seconds / SECONDS_PER_DAY;
        return std::format("{} day{}", days, days > 1 ? "s" : "");
    }
    else if (seconds % SECONDS_PER_HOUR == 0 && seconds >= SECONDS_PER_HOUR)
    {
        int64_t hours = seconds / SECONDS_PER_HOUR;
        return std::format("{} hour{}", hours, hours > 1 ? "s" : "");
    }
    else if (seconds % SECONDS_PER_MINUTE == 0 && seconds >= SECONDS_PER_MINUTE)
    {
        int64_t minutes = seconds / SECONDS_PER_MINUTE;
        return std::format("{} minute{}", minutes, minutes > 1 ? "s" : "");
    }
    else
    {
        return std::format("{} second{}", seconds, seconds > 1 ? "s" : "");
    }
}

std::string Time::FormatTimestamp(int64_t timestamp)
{
    if (timestamp == 0)
    {
        return "Never";
    }

    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm tm;

#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif

    return std::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec);
}

bool Time::IsExpired(int64_t expiresAt)
{
    if (expiresAt == 0) // Permanent
    {
        return false;
    }

    return Now() >= expiresAt;
}

int64_t Time::GetExpirationTime(int64_t durationSeconds)
{
    if (durationSeconds == 0) // Permanent
    {
        return 0;
    }

    return Now() + durationSeconds;
}

} // namespace utils
