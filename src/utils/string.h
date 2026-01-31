#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace utils {

/**
 * @brief String utility class for common string operations
 */
class String
{
public:
    /**
     * @brief Convert string to lowercase
     * @param str Input string
     * @return Lowercase string
     */
    static std::string ToLower(const std::string& str);

    /**
     * @brief Convert string to uppercase
     * @param str Input string
     * @return Uppercase string
     */
    static std::string ToUpper(const std::string& str);

    /**
     * @brief Trim whitespace from both ends of string
     * @param str Input string
     * @return Trimmed string
     */
    static std::string Trim(const std::string& str);

    /**
     * @brief Trim whitespace from left side of string
     * @param str Input string
     * @return Left-trimmed string
     */
    static std::string TrimLeft(const std::string& str);

    /**
     * @brief Trim whitespace from right side of string
     * @param str Input string
     * @return Right-trimmed string
     */
    static std::string TrimRight(const std::string& str);

    /**
     * @brief Split string by delimiter
     * @param str Input string
     * @param delimiter Delimiter character
     * @return Vector of string parts
     */
    static std::vector<std::string> Split(const std::string& str, char delimiter);

    /**
     * @brief Join vector of strings with delimiter
     * @param parts Vector of string parts
     * @param delimiter Delimiter string
     * @return Joined string
     */
    static std::string Join(const std::vector<std::string>& parts, const std::string& delimiter);

    /**
     * @brief Check if string starts with prefix
     * @param str Input string
     * @param prefix Prefix to check
     * @return true if starts with prefix
     */
    static bool StartsWith(const std::string& str, const std::string& prefix);

    /**
     * @brief Check if string ends with suffix
     * @param str Input string
     * @param suffix Suffix to check
     * @return true if ends with suffix
     */
    static bool EndsWith(const std::string& str, const std::string& suffix);

    /**
     * @brief Check if string contains substring (case-insensitive)
     * @param str Input string
     * @param substr Substring to find
     * @return true if contains substring
     */
    static bool ContainsIgnoreCase(const std::string& str, const std::string& substr);

    /**
     * @brief Replace all occurrences of a substring
     * @param str Input string
     * @param from Substring to replace
     * @param to Replacement substring
     * @return String with replacements
     */
    static std::string ReplaceAll(const std::string& str, const std::string& from, const std::string& to);

    /**
     * @brief Check if string is numeric
     * @param str Input string
     * @return true if string contains only digits
     */
    static bool IsNumeric(const std::string& str);

    /**
     * @brief Parse target selector (e.g., "@all", "@me", "#3", "playerName")
     * @param target Target string
     * @return Parsed target type and value
     */
    struct TargetInfo
    {
        enum class Type
        {
            ALL,      // @all
            ME,       // @me
            INDEX,    // #3 (slot index)
            NAME,     // Partial name match
            STEAMID   // SteamID64/SteamID3/SteamID
        };

        Type type;
        std::string value;
    };

    static TargetInfo ParseTarget(const std::string& target);
};

} // namespace utils
