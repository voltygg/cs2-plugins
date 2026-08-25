#include "MapQuery.hpp"

#include <algorithm>
#include <cctype>
#include <format>

namespace AdminSystem::Maps
{

namespace
{

std::string Lower(std::string_view text)
{
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

}  // namespace

MapLookup FindMap(const std::vector<MapEntry>& maps, std::string_view query)
{
    const std::string needle = Lower(query);
    if (needle.empty())
        return {};

    std::vector<std::size_t> hits;
    auto tier = [&](auto&& matches) {
        hits.clear();
        for (std::size_t i = 0; i < maps.size(); ++i)
        {
            if (matches(Lower(maps[i].Name)) || (!maps[i].DisplayName.empty() && matches(Lower(maps[i].DisplayName))))
                hits.push_back(i);
        }
        return !hits.empty();
    };

    tier([&](const std::string& name) { return name == needle; }) ||
        tier([&](const std::string& name) { return name.starts_with(needle); }) ||
        tier([&](const std::string& name) { return name.find(needle) != std::string::npos; });

    if (hits.empty())
        return {};
    if (hits.size() > 1)
        return {.Result = MapMatch::Ambiguous, .Index = 0, .Count = hits.size()};
    return {.Result = MapMatch::Unique, .Index = hits.front(), .Count = 1};
}

std::string ValidateMapEntry(const MapEntry& entry)
{
    if (entry.Name.empty())
        return "name must be non-empty";
    if (entry.MinPlayers < 0 || entry.MaxPlayers < 0)
        return "minPlayers and maxPlayers must not be negative";
    // A zero max means "no limit", so only a real upper bound is compared.
    if (entry.MaxPlayers > 0 && entry.MaxPlayers < entry.MinPlayers)
        return std::format("maxPlayers {} is below minPlayers {}", entry.MaxPlayers, entry.MinPlayers);
    return {};
}

}  // namespace AdminSystem::Maps
