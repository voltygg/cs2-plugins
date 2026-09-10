#pragma once

#include <VoltMod/Api.hpp>
#include <VoltMod/Runtime.hpp>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>

namespace AdminSystem::Admin::Menu
{

/** Localized duration label ("permanent" / "5 minutes") in @p slot's language, fed from this
 *  plugin's `duration.*` translations. */
inline std::string DurationLabel(VoltMod::Translations& translations, int seconds, int slot)
{
    return VoltMod::Time::FormatDurationLabel(seconds, {.Permanent = translations.Get("duration.perm", slot),
                                                        .Days = translations.Get("duration.unitDays", slot),
                                                        .Hours = translations.Get("duration.unitHours", slot),
                                                        .Minutes = translations.Get("duration.unitMinutes", slot),
                                                        .Seconds = translations.Get("duration.unitSeconds", slot)});
}

/** Human-readable expiry for a punishment ("permanent" or "expires in ...") in the admin's language. */
inline std::string ExpiryLabel(VoltMod::Translations& translations, int64_t expiresAt, int adminSlot)
{
    return VoltMod::Time::FormatExpiry(expiresAt, VoltMod::Time::Now(), translations.Get("duration.perm", adminSlot),
                                       translations.Get("unban.expiresIn", adminSlot));
}

/** "Confirm: Ban" - the confirm-step title every flow that asks before acting shares. */
inline std::string ConfirmTitle(VoltMod::Translations& translations, std::string_view actionKey, int slot)
{
    return std::format("{}: {}", translations.Get("punish.confirmTitle", slot), translations.Get(actionKey, slot));
}

}  // namespace AdminSystem::Admin::Menu
