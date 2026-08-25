#include "WeaponCatalog.hpp"

#include <algorithm>
#include <cctype>

namespace AdminSystem::Weapons
{

namespace
{

constexpr std::string_view ItemPrefix = "weapon_";

std::string Lower(std::string_view text)
{
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

/** Drop the `weapon_` prefix so "ak47" and "weapon_ak47" compare the same. */
std::string_view Bare(std::string_view name)
{
    return name.starts_with(ItemPrefix) ? name.substr(ItemPrefix.size()) : name;
}

}  // namespace

WeaponLookup FindWeapon(const std::vector<WeaponEntry>& weapons, std::string_view query)
{
    const std::string needle(Bare(Lower(query)));
    if (needle.empty())
        return {};

    std::vector<std::size_t> hits;
    auto tier = [&](auto&& matches) {
        hits.clear();
        for (std::size_t i = 0; i < weapons.size(); ++i)
        {
            const std::string item(Bare(Lower(weapons[i].Item)));
            if (matches(item) || (!weapons[i].Name.empty() && matches(Lower(weapons[i].Name))))
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
        return {.Result = WeaponMatch::Ambiguous, .Index = 0, .Count = hits.size()};
    return {.Result = WeaponMatch::Unique, .Index = hits.front(), .Count = 1};
}

std::string ValidateWeaponEntry(const WeaponEntry& entry)
{
    if (entry.Item.empty())
        return "item must be non-empty";
    // Anything else would be handed to GiveNamedItem as an arbitrary classname.
    if (!std::string_view(entry.Item).starts_with(ItemPrefix))
        return "item must be an entity classname starting with 'weapon_'";
    return {};
}

}  // namespace AdminSystem::Weapons
