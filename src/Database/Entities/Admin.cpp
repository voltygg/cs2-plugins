#include "Admin.hpp"

namespace AdminSystem::Database
{

bool Admin::HasFlag(char flag) const
{
    if (FlagBits & FlagToBit('z'))  // Root has all flags
    {
        return true;
    }

    return (FlagBits & FlagToBit(flag)) != 0;
}

void Admin::BuildFlagBits()
{
    FlagBits = 0;
    for (char c : Flags)
    {
        FlagBits |= FlagToBit(c);
    }
}

uint32_t Admin::FlagToBit(char flag)
{
    if (flag >= 'a' && flag <= 'z')
    {
        return 1u << (flag - 'a');
    }

    return 0;
}

}  // namespace AdminSystem::Database
