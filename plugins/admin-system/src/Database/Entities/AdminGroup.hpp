#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace AdminSystem::Database
{

/** Database entity representing a named admin group with flags, immunity, and inheritance. */
struct AdminGroup
{
    int64_t Id = 0;
    std::string Name;
    std::string Flags;
    int32_t Immunity = 0;
    std::vector<std::string> Inherits;

    /** Optional chat styling, applied when an admin in this group speaks. Empty = no override. */
    std::string ChatPrefix;   /**< E.g., "[ADMIN]". Empty disables prefixing for this group. */
    std::string PrefixColor;  /**< Color name (see VoltMod::ChatColors::ParseNamed). */
    std::string NameColor;    /**< Color name for the speaker's display name. */
    std::string MessageColor; /**< Color name for the spoken message body. */

    int64_t CreatedAt = 0;
    int64_t UpdatedAt = 0;

    uint32_t FlagBits = 0;

    bool HasFlag(char flag) const;
    void BuildFlagBits();
};

}  // namespace AdminSystem::Database
