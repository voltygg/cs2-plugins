#include "string.h"

#include "steamid.h"

#include <sstream>

namespace utils
{

std::string String::ToLower(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string String::ToUpper(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::string String::TrimLeft(const std::string& str)
{
    auto it = std::find_if(str.begin(), str.end(), [](unsigned char c) { return !std::isspace(c); });
    return std::string(it, str.end());
}

std::string String::TrimRight(const std::string& str)
{
    auto it = std::find_if(str.rbegin(), str.rend(), [](unsigned char c) { return !std::isspace(c); });
    return std::string(str.begin(), it.base());
}

std::string String::Trim(const std::string& str)
{
    return TrimLeft(TrimRight(str));
}

std::vector<std::string> String::Split(const std::string& str, char delimiter)
{
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;

    while (std::getline(ss, item, delimiter))
    {
        result.push_back(item);
    }

    return result;
}

std::string String::Join(const std::vector<std::string>& parts, const std::string& delimiter)
{
    if (parts.empty())
    {
        return "";
    }

    std::string result = parts[0];
    for (size_t i = 1; i < parts.size(); ++i)
    {
        result += delimiter + parts[i];
    }

    return result;
}

bool String::StartsWith(const std::string& str, const std::string& prefix)
{
    if (prefix.length() > str.length())
    {
        return false;
    }

    return str.substr(0, prefix.length()) == prefix;
}

bool String::EndsWith(const std::string& str, const std::string& suffix)
{
    if (suffix.length() > str.length())
    {
        return false;
    }

    return str.substr(str.length() - suffix.length()) == suffix;
}

bool String::ContainsIgnoreCase(const std::string& str, const std::string& substr)
{
    std::string lowerStr = ToLower(str);
    std::string lowerSubstr = ToLower(substr);
    return lowerStr.find(lowerSubstr) != std::string::npos;
}

std::string String::ReplaceAll(const std::string& str, const std::string& from, const std::string& to)
{
    if (from.empty())
    {
        return str;
    }

    std::string result = str;
    size_t pos = 0;

    while ((pos = result.find(from, pos)) != std::string::npos)
    {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }

    return result;
}

bool String::IsNumeric(const std::string& str)
{
    if (str.empty())
    {
        return false;
    }

    return std::all_of(str.begin(), str.end(), [](unsigned char c) { return std::isdigit(c); });
}

String::TargetInfo String::ParseTarget(const std::string& target)
{
    TargetInfo info;

    if (target.empty())
    {
        info.type = TargetInfo::Type::NAME;
        info.value = "";
        return info;
    }

    // @all selector
    if (target == "@all" || target == "@*")
    {
        info.type = TargetInfo::Type::ALL;
        info.value = target;
        return info;
    }

    // @me selector
    if (target == "@me")
    {
        info.type = TargetInfo::Type::ME;
        info.value = target;
        return info;
    }

    // #<index> selector
    if (target[0] == '#' && target.length() > 1)
    {
        std::string indexStr = target.substr(1);
        if (IsNumeric(indexStr))
        {
            info.type = TargetInfo::Type::INDEX;
            info.value = indexStr;
            return info;
        }
    }

    // Check if it's a SteamID64 (all numeric, very long)
    if (IsNumeric(target) && target.length() >= 15)
    {
        try
        {
            int64_t steamid64 = std::stoll(target);
            if (SteamID::IsValid(steamid64))
            {
                info.type = TargetInfo::Type::STEAMID;
                info.value = target;
                return info;
            }
        }
        catch (...)
        {
            // Fall through to name matching
        }
    }

    // Check if it's a SteamID3 format
    if (StartsWith(target, "[U:1:"))
    {
        auto steamid64 = SteamID::FromSteamID3(target);
        if (steamid64.has_value())
        {
            info.type = TargetInfo::Type::STEAMID;
            info.value = std::to_string(*steamid64);
            return info;
        }
    }

    // Check if it's a legacy SteamID format
    if (StartsWith(target, "STEAM_"))
    {
        auto steamid64 = SteamID::FromSteamID(target);
        if (steamid64.has_value())
        {
            info.type = TargetInfo::Type::STEAMID;
            info.value = std::to_string(*steamid64);
            return info;
        }
    }

    // Default to name matching
    info.type = TargetInfo::Type::NAME;
    info.value = target;
    return info;
}

}  // namespace utils
