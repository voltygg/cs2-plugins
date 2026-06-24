#pragma once

#include <cstdint>
#include <string>

namespace AdminSystem::Core::ChatFormat
{

/**
 * Render a Unix expiry timestamp as a human-readable suffix for mute/ban notices,
 * localized to @p slot (the recipient). Returns the translated "permanent" word for
 * `expiresAt <= 0`, otherwise "{muteNoticeExpiresIn} {duration}".
 */
std::string FormatExpiry(int64_t expiresAt, int slot);

}  // namespace AdminSystem::Core::ChatFormat
