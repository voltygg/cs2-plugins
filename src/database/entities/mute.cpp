#include "Mute.hpp"
#include "../../Utils/TimeUtils.hpp"

namespace AdminSystem::Database {

bool Mute::IsExpired() const
{
    return !IsPermanent() && Utils::TimeUtils::IsExpired(ExpiresAt);
}

} // namespace AdminSystem::Database
