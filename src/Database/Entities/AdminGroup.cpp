#include "AdminGroup.hpp"

#include "Admin.hpp"

namespace AdminSystem::Database
{

bool AdminGroup::HasFlag(char flag) const
{
    if (FlagBits & Admin::FlagToBit('z'))
        return true;
    return (FlagBits & Admin::FlagToBit(flag)) != 0;
}

void AdminGroup::BuildFlagBits()
{
    FlagBits = 0;
    for (char c : Flags)
        FlagBits |= Admin::FlagToBit(c);
}

}  // namespace AdminSystem::Database
