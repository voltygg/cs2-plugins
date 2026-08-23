#include "Ban.hpp"

#include <CS2Kit/Core/TimeUtils.hpp>

namespace AdminSystem::Database
{

using namespace CS2Kit::Core;

bool Ban::IsExpired() const
{
    return !IsPermanent() && TimeUtils::IsExpired(ExpiresAt);
}

}  // namespace AdminSystem::Database
