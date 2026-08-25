#include "KickNoticeCore.hpp"

#include <array>

namespace AdminSystem::Punishments
{

std::string JoinBanNotice(const BanNoticeParts& parts, std::string_view separator)
{
    const std::array<const std::string*, 3> pieces{&parts.Reason, &parts.Expiry, &parts.Appeal};

    std::string notice;
    for (const auto* piece : pieces)
    {
        if (piece->empty())
            continue;
        if (!notice.empty())
            notice += separator;
        notice += *piece;
    }
    return notice;
}

}  // namespace AdminSystem::Punishments
