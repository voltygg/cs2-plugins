#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace AdminSystem::Database {

/** Database entity representing a named admin group with flags, immunity, and inheritance. */
struct AdminGroup {
    int64_t Id = 0;
    std::string Name;
    std::string Flags;
    int32_t Immunity = 0;
    std::vector<std::string> Inherits;
    int64_t CreatedAt = 0;
    int64_t UpdatedAt = 0;

    uint32_t FlagBits = 0;

    bool HasFlag(char flag) const;
    void BuildFlagBits();
};

} // namespace AdminSystem::Database
