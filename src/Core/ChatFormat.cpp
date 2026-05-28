#include "ChatFormat.hpp"

#include <CS2Kit/Utils/TimeUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>

namespace AdminSystem::Core::ChatFormat
{

using CS2Kit::Utils::TimeUtils;
using CS2Kit::Utils::Translations;

std::string FormatExpiry(int64_t expiresAt)
{
    if (expiresAt <= 0)
        return Translations::Instance().Get("muteNotice.permanent");

    int64_t remaining = expiresAt - TimeUtils::Now();
    if (remaining <= 0)
        return TimeUtils::FormatDuration(0);

    return std::format("{} {}", Translations::Instance().Get("muteNotice.expiresIn"),
                       TimeUtils::FormatDuration(remaining));
}

}  // namespace AdminSystem::Core::ChatFormat
