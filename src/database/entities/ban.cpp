#include "ban.h"
#include "../../utils/time.h"

namespace database {

bool Ban::IsExpired() const
{
    return utils::Time::IsExpired(expires_at);
}

} // namespace database
