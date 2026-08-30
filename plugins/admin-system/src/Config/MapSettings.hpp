#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace AdminSystem::Config
{

/** Raw map entry, validated into Maps::MapEntry during load. */
struct MapConfigEntry
{
    std::string name;
    std::string displayName;
    uint64_t workshopId = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MapConfigEntry, name, displayName, workshopId)

/** The map vote an admin opens from the Map menu. */
struct MapVoteSettings
{
    /** Share of the ballots cast that must be yes; a strict majority of it is required. */
    double successRatio = 0.6;
    /** How long the panel stays open. */
    int durationSec = 20;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MapVoteSettings, successRatio, durationSec)

/** Maps an admin may switch to. The engine offers no usable list of its own, so this is it. The
 *  defaults are the active duty group, so a settings file predating this section still opens a
 *  usable Map menu. */
struct MapSettings
{
    std::vector<MapConfigEntry> cycle = {
        {"de_dust2", "Dust II"}, {"de_mirage", "Mirage"},   {"de_inferno", "Inferno"},
        {"de_nuke", "Nuke"},     {"de_ancient", "Ancient"}, {"de_anubis", "Anubis"},
    };
    MapVoteSettings vote;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MapSettings, cycle, vote)

}  // namespace AdminSystem::Config
