#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace AdminSystem::Database
{

/** Database entity representing an administrator with permission flags and group membership. */
struct Admin
{
    int64_t Id = 0;
    int64_t SteamId = 0;
    std::string Name;
    std::vector<std::string> Groups;
    std::string Flags;
    int32_t Immunity = 0;

    /** Per-admin chat overrides. Empty color strings fall back to the admin's group. */
    bool DisplayPrefix = true;
    std::string NameColor;
    std::string MessageColor;

    int64_t CreatedAt = 0;
    int64_t UpdatedAt = 0;

    // Use bitmask for O(1) flag checks: 'a'=bit0, 'b'=bit1, ... 'z'=bit25
    uint32_t FlagBits = 0;

    bool HasFlag(char flag) const;
    void BuildFlagBits();
    static uint32_t FlagToBit(char flag);
};

}  // namespace AdminSystem::Database
