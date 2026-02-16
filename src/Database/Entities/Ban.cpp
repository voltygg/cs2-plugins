#include "Ban.hpp"
#include "../../Utils/TimeUtils.hpp"

namespace AdminSystem::Database {

using namespace AdminSystem::Utils;

bool Ban::IsExpired() const
{
    return !IsPermanent() && TimeUtils::IsExpired(ExpiresAt);
}

} // namespace AdminSystem::Database
