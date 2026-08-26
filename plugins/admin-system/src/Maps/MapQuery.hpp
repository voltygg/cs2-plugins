#pragma once

#include <cstdint>
#include <string>

namespace AdminSystem::Maps
{

/** One configured map. A non-zero @ref WorkshopId makes it a workshop map addressed by id. */
struct MapEntry
{
    std::string Name;
    std::string DisplayName; /**< menu label; falls back to Name when empty */
    uint64_t WorkshopId = 0;

    const std::string& Label() const { return DisplayName.empty() ? Name : DisplayName; }
};

/**
 * Why @p entry cannot be offered, or an empty string when it is fine.
 *
 * Validation only - it cannot tell whether a map file exists, which is the engine's job
 * (VoltMod::Sdk::MapService::IsValid).
 *
 * Kept free of the framework and the SDK so it is unit-testable.
 */
std::string ValidateMapEntry(const MapEntry& entry);

}  // namespace AdminSystem::Maps
