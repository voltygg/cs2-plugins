#include "MapQuery.hpp"

namespace AdminSystem::Maps
{

std::string ValidateMapEntry(const MapEntry& entry)
{
    if (entry.Name.empty())
        return "name must be non-empty";
    return {};
}

}  // namespace AdminSystem::Maps
