#pragma once

#include <cstdint>
#include <string>

namespace AdminSystem::Core::ChatFormat
{

/**
 * Render a Unix expiry timestamp as a human-readable suffix for mute/ban notices.
 * Returns the translated "permanent" word for `expiresAt <= 0`, otherwise
 * "{muteNoticeExpiresIn} {duration}" with the remaining time formatted via TimeUtils.
 */
std::string FormatExpiry(int64_t expiresAt);

}  // namespace AdminSystem::Core::ChatFormat
