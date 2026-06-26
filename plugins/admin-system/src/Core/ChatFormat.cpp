#include "ChatFormat.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>

using CS2Kit::Core::Engine;

namespace AdminSystem::Core::ChatFormat
{

using CS2Kit::Utils::TimeUtils;
using CS2Kit::Utils::Translations;

std::string FormatExpiry(int64_t expiresAt, int slot)
{
    if (expiresAt <= 0)
        return Engine().Translations.Get("muteNotice.permanent", slot);

    int64_t remaining = expiresAt - TimeUtils::Now();
    if (remaining <= 0)
        return TimeUtils::FormatDuration(0);

    return std::format("{} {}", Engine().Translations.Get("muteNotice.expiresIn", slot),
                       TimeUtils::FormatDuration(remaining));
}

}  // namespace AdminSystem::Core::ChatFormat
