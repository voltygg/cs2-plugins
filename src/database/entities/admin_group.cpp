#include "admin_group.h"

namespace database
{

bool AdminGroup::HasFlag(char flag) const
{
    // Root flag 'z' grants all permissions
    if (flags.find('z') != std::string::npos)
    {
        return true;
    }

    return flags.find(flag) != std::string::npos;
}

}  // namespace database
