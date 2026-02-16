#include "Ban.hpp"
#include "../../Utils/TimeUtils.hpp"

namespace AdminSystem::Database {

bool Ban::IsExpired() const
{
    return !IsPermanent() && Utils::TimeUtils::IsExpired(ExpiresAt);
}

} // namespace AdminSystem::Database
