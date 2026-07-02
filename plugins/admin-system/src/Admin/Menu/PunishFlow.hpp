#pragma once

#include "../../Punishments/PunishType.hpp"

#include <CS2Kit/Menu/Menu.hpp>
#include <cstdint>
#include <memory>
#include <string>

namespace AdminSystem::Admin::Menu
{

/**
 * Everything a menu-issued punishment accumulates while walking the steps
 * (duration -> reason -> confirm). Copied by value through the menu callbacks.
 */
struct PendingPunishment
{
    Punishments::PunishType Type = Punishments::PunishType::Kick;
    int TargetSlot = -1;
    /** Captured at selection and re-verified at confirm, so slot reuse can't hit the wrong player. */
    int64_t TargetSteamId = 0;
    std::string TargetName;
    int DurationSec = 0;  // 0 = permanent; ignored for Kick/Warn
    std::string Reason;
};

/**
 * Entry into the punish flow for @p pending's type: duration -> reason -> confirm for timed
 * punishments, reason -> confirm for kick/warn. Confirm re-validates the target, then issues.
 */
std::shared_ptr<::CS2Kit::Menu::Menu> BuildFirstStep(int adminSlot, PendingPunishment pending);

/** Quick Punish list of the configured templates the admin may use on this target. */
std::shared_ptr<::CS2Kit::Menu::Menu> BuildQuickPunishMenu(int adminSlot, int targetSlot);

/** True if at least one configured template is usable by @p adminSlot against @p targetSlot. */
bool AnyTemplateUsable(int adminSlot, int targetSlot);

}  // namespace AdminSystem::Admin::Menu
