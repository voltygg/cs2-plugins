#pragma once

#include <string>
#include <string_view>

namespace AdminSystem::Punishments
{

/** Localized, already-rendered pieces of the notice a banned player sees on the disconnect screen. */
struct BanNoticeParts
{
    std::string Reason;
    std::string Expiry; /**< empty when `punishments.appeal.showExpiry` is off */
    std::string Appeal; /**< empty when no appeal URL is configured */
};

/**
 * Join the notice pieces, dropping empty ones.
 *
 * Deliberately free of the framework and the SDK: the connect-time reject and an online ban
 * both render through here, so the two paths cannot drift, and the assembly is unit-testable.
 * Dropping empties is the point - an unconfigured appeal URL must not leave a dangling
 * separator or a bare "Appeal:" label with no link behind it.
 */
std::string JoinBanNotice(const BanNoticeParts& parts, std::string_view separator = " | ");

}  // namespace AdminSystem::Punishments
