#include "mute.h"
#include "../../utils/time.h"

namespace database {

bool Mute::IsExpired() const
{
    return utils::Time::IsExpired(expires_at);
}

} // namespace database
