#pragma once

#include <VoltMod/Api.hpp>
#include <VoltMod/Runtime.hpp>
#include <cstdint>
#include <string>

namespace AdminSystem::Admin::Menu
{

/** Localized duration label ("permanent" / "5 minutes") in @p slot's language, fed from this
 *  plugin's `duration.*` translations. */
inline std::string DurationLabel(VoltMod::Translations& tr, int seconds, int slot)
{
    return VoltMod::Time::FormatDurationLabel(seconds, {.Permanent = tr.Get("duration.perm", slot),
                                                             .Days = tr.Get("duration.unitDays", slot),
                                                             .Hours = tr.Get("duration.unitHours", slot),
                                                             .Minutes = tr.Get("duration.unitMinutes", slot),
                                                             .Seconds = tr.Get("duration.unitSeconds", slot)});
}

/** Human-readable expiry for a punishment ("permanent" or "expires in ...") in the admin's language. */
inline std::string ExpiryLabel(VoltMod::Translations& tr, int64_t expiresAt, int adminSlot)
{
    return VoltMod::Time::FormatExpiry(expiresAt, VoltMod::Time::Now(), tr.Get("duration.perm", adminSlot),
                                            tr.Get("unban.expiresIn", adminSlot));
}

}  // namespace AdminSystem::Admin::Menu
