#pragma once

#include <cstdint>
#include <string>

namespace VoltMod::Core
{
class Translations;
}

namespace AdminSystem::Core
{
struct AppealSettings;
}

namespace AdminSystem::Punishments
{

/**
 * Render the disconnect-screen text for a ban: the reason, how long it lasts, and where to
 * appeal it.
 *
 * Both kick paths for a ban (the connect-time reject and the kick that follows an online ban)
 * go through here so a player reads the same notice either way.
 *
 * @param slot the target's slot, so the notice is in their language. Pass -1 for the server
 *        language when the seat is already gone.
 * @param expiresAt Unix seconds, or 0 for a permanent ban.
 */
std::string BuildBanNotice(VoltMod::Core::Translations& translations, const Core::AppealSettings& appeal,
                           const std::string& reason, int64_t expiresAt, int64_t targetSteamId, int slot);

}  // namespace AdminSystem::Punishments
