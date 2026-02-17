#include "Gag.hpp"

#include <CS2Kit/Utils/TimeUtils.hpp>

namespace AdminSystem::Database
{

using namespace CS2Kit::Utils;

bool Gag::IsExpired() const
{
    return !IsPermanent() && TimeUtils::IsExpired(ExpiresAt);
}

}  // namespace AdminSystem::Database
