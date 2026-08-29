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

/** "Confirm: Ban" - the confirm-step title every flow that asks before acting shares. */
inline std::string ConfirmTitle(VoltMod::Translations& tr, std::string_view actionKey, int slot)
{
    return std::format("{}: {}", tr.Get("punish.confirmTitle", slot), tr.Get(actionKey, slot));
}

/** The confirm button's label. A flow runs for one player, so it resolves once here. */
inline std::string ConfirmLabel(VoltMod::Translations& tr, int slot)
{
    return tr.Get("punish.confirm", slot);
}

/** The cancel button's label, resolved for the one player the flow runs for. */
inline std::string CancelLabel(VoltMod::Translations& tr, int slot)
{
    return tr.Get("punish.cancel", slot);
}

}  // namespace AdminSystem::Admin::Menu
