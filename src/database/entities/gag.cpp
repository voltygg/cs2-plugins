#include "gag.h"
#include "../../utils/time.h"

namespace database {

bool Gag::IsExpired() const
{
    return utils::Time::IsExpired(expires_at);
}

} // namespace database
