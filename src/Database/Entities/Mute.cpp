#include "Mute.hpp"

#include "../../Utils/TimeUtils.hpp"

using namespace AdminSystem::Utils;

namespace AdminSystem::Database
{

bool Mute::IsExpired() const
{
    return !IsPermanent() && TimeUtils::IsExpired(ExpiresAt);
}

}  // namespace AdminSystem::Database
