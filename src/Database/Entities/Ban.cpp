#include "Ban.hpp"

#include "../../../vendor/cs2-kit/src/Utils/TimeUtils.hpp"

namespace AdminSystem::Database
{

using namespace CS2Kit::Utils;

bool Ban::IsExpired() const
{
    return !IsPermanent() && TimeUtils::IsExpired(ExpiresAt);
}

}  // namespace AdminSystem::Database
