#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace AdminSystem::Maps
{

/** One configured map. A non-zero @ref WorkshopId makes it a workshop map addressed by id. */
struct MapEntry
{
    std::string Name;
    std::string DisplayName; /**< menu label; falls back to Name when empty */
    uint64_t WorkshopId = 0;
    int MinPlayers = 0;
    int MaxPlayers = 0; /**< 0 = no upper limit */

    const std::string& Label() const { return DisplayName.empty() ? Name : DisplayName; }
};

enum class MapMatch
{
    None,
    Unique,
    Ambiguous,
};

struct MapLookup
{
    MapMatch Result = MapMatch::None;
    std::size_t Index = 0; /**< only meaningful when Result is Unique */
    std::size_t Count = 0; /**< how many entries matched, for an ambiguous reply */
};

/**
 * Resolve a player-typed map name.
 *
 * Tiered the same way player targeting is (exact, then prefix, then substring, all
 * case-insensitive), so `!map dust` picks `de_dust2` over `de_dust2_night` and the two commands
 * behave alike. Both @ref MapEntry::Name and @ref MapEntry::DisplayName are matched, because an
 * operator who labels a map "Dust II" should not have to type `de_dust2`.
 *
 * Kept free of the framework and the SDK so it is unit-testable.
 */
MapLookup FindMap(const std::vector<MapEntry>& maps, std::string_view query);

/**
 * Why @p entry cannot be offered, or an empty string when it is fine.
 *
 * Validation only - it cannot tell whether a map file exists, which is the engine's job
 * (VoltMod::Sdk::MapService::IsValid).
 */
std::string ValidateMapEntry(const MapEntry& entry);

}  // namespace AdminSystem::Maps
