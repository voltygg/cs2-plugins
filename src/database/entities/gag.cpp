#include "Gag.hpp"
#include "../../Utils/TimeUtils.hpp"

namespace AdminSystem::Database {

bool Gag::IsExpired() const
{
    return !IsPermanent() && Utils::TimeUtils::IsExpired(ExpiresAt);
}

} // namespace AdminSystem::Database
