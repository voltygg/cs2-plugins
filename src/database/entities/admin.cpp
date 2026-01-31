#include "admin.h"
#include <algorithm>

namespace database {

bool Admin::HasFlag(char flag) const
{
    // Root flag 'z' grants all permissions
    if (flags.find('z') != std::string::npos)
    {
        return true;
    }

    return flags.find(flag) != std::string::npos;
}

bool Admin::HasAllFlags(const std::string& requiredFlags) const
{
    // Root flag 'z' grants all permissions
    if (flags.find('z') != std::string::npos)
    {
        return true;
    }

    for (char flag : requiredFlags)
    {
        if (flags.find(flag) == std::string::npos)
        {
            return false;
        }
    }

    return true;
}

bool Admin::HasAnyFlag(const std::string& anyFlags) const
{
    // Root flag 'z' grants all permissions
    if (flags.find('z') != std::string::npos)
    {
        return true;
    }

    for (char flag : anyFlags)
    {
        if (flags.find(flag) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

bool Admin::IsRoot() const
{
    return HasFlag('z');
}

} // namespace database
