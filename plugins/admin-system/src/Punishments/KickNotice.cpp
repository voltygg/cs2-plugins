#include "KickNotice.hpp"

#include "../Core/Config.hpp"
#include "KickNoticeCore.hpp"

#include <VoltMod/Core/StringUtils.hpp>
#include <VoltMod/Core/TimeUtils.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <format>

namespace AdminSystem::Punishments
{

using VoltMod::Core::StringUtils;
using VoltMod::Core::TimeUtils;

std::string BuildBanNotice(VoltMod::Core::Translations& translations, const Core::AppealSettings& appeal,
                           const std::string& reason, int64_t expiresAt, int64_t targetSteamId, int slot)
{
    BanNoticeParts parts;
    parts.Reason = reason;

    if (appeal.showExpiry)
        parts.Expiry =
            TimeUtils::FormatExpiry(expiresAt, TimeUtils::Now(), translations.Get("kickNotice.permanent", slot),
                                    translations.Get("kickNotice.expiresIn", slot));

    if (!appeal.url.empty())
    {
        // The SteamID lets an appeal form identify the case without the player copying anything
        // down; the row id is not substituted because the insert is async and is still 0 here.
        auto url = StringUtils::SubstituteTokens(appeal.url, {{"steamId", std::to_string(targetSteamId)}});
        parts.Appeal = std::format("{} {}", translations.Get("kickNotice.appeal", slot), url);
    }

    return JoinBanNotice(parts);
}

}  // namespace AdminSystem::Punishments
