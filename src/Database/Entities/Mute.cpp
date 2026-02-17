#include "Mute.hpp"

#include <CS2Kit/Utils/TimeUtils.hpp>

using namespace CS2Kit::Utils;

namespace AdminSystem::Database
{

bool Mute::IsExpired() const
{
    return !IsPermanent() && TimeUtils::IsExpired(ExpiresAt);
}

}  // namespace AdminSystem::Database
