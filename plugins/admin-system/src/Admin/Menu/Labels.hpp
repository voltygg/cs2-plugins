#pragma once

#include <CS2Kit/Api.hpp>
#include <CS2Kit/App/Services.hpp>
#include <cstdint>
#include <string>

namespace AdminSystem::Admin::Menu
{

/** Localized duration label ("permanent" / "5 minutes") in @p slot's language, fed from this
 *  plugin's `duration.*` translations. */
inline std::string DurationLabel(int seconds, int slot)
{
    auto& tr = CS2Kit::Engine().Utils.Translations;
    return CS2Kit::TimeUtils::FormatDurationLabel(seconds, {.Permanent = tr.Get("duration.perm", slot),
                                                            .Days = tr.Get("duration.unitDays", slot),
                                                            .Hours = tr.Get("duration.unitHours", slot),
                                                            .Minutes = tr.Get("duration.unitMinutes", slot),
                                                            .Seconds = tr.Get("duration.unitSeconds", slot)});
}

/** Human-readable expiry for a punishment ("permanent" or "expires in ...") in the admin's language. */
inline std::string ExpiryLabel(int64_t expiresAt, int adminSlot)
{
    auto& tr = CS2Kit::Engine().Utils.Translations;
    return CS2Kit::TimeUtils::FormatExpiry(expiresAt, CS2Kit::TimeUtils::Now(), tr.Get("duration.perm", adminSlot),
                                           tr.Get("unban.expiresIn", adminSlot));
}

}  // namespace AdminSystem::Admin::Menu
