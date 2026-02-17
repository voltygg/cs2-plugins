#include "Mute.hpp"

#include "../../../vendor/cs2-kit/src/Utils/TimeUtils.hpp"

using namespace CS2Kit::Utils;

namespace AdminSystem::Database
{

bool Mute::IsExpired() const
{
    return !IsPermanent() && TimeUtils::IsExpired(ExpiresAt);
}

}  // namespace AdminSystem::Database
