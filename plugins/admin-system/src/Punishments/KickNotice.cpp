#include "KickNotice.hpp"

#include "../Core/Config.hpp"

#include <VoltMod/Core/Strings.hpp>
#include <VoltMod/Core/Time.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <format>

namespace AdminSystem::Punishments
{

using VoltMod::Strings;
using VoltMod::Time;

/** Between the reason, the expiry, and the appeal link on the disconnect screen. */
static constexpr const char* PieceSeparator = " | ";

std::string BuildBanNotice(VoltMod::Translations& translations, const Core::AppealSettings& appeal,
                           const std::string& reason, int64_t expiresAt, int64_t targetSteamId, int slot)
{
    std::string expiry;
    if (appeal.showExpiry)
        expiry = Time::FormatExpiry(expiresAt, Time::Now(), translations.Get("kickNotice.permanent", slot),
                                    translations.Get("kickNotice.expiresIn", slot));

    std::string appealLine;
    if (!appeal.url.empty())
    {
        // The SteamID lets an appeal form identify the case without the player copying anything
        // down; the row id is not substituted because the insert is async and is still 0 here.
        auto url = Strings::SubstituteTokens(appeal.url, {{"steamId", std::to_string(targetSteamId)}});
        appealLine = std::format("{} {}", translations.Get("kickNotice.appeal", slot), url);
    }

    // Empty pieces are dropped rather than joined: an unconfigured appeal URL or a suppressed
    // expiry must not leave a dangling separator behind it.
    return Strings::JoinNonEmpty({reason, expiry, appealLine}, PieceSeparator);
}

}  // namespace AdminSystem::Punishments
