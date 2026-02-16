#include "Mute.hpp"
#include "../../Utils/TimeUtils.hpp"

namespace AdminSystem::Database {

using namespace AdminSystem::Utils;

bool Mute::IsExpired() const
{
    return !IsPermanent() && TimeUtils::IsExpired(ExpiresAt);
}

} // namespace AdminSystem::Database
