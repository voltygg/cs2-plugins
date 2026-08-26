#include "WeaponCatalog.hpp"

#include <string_view>

namespace AdminSystem::Weapons
{

namespace
{
constexpr std::string_view ItemPrefix = "weapon_";
}  // namespace

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
