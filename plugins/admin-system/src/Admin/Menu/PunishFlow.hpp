#pragma once

#include "../../Core/App.hpp"
#include "../../Punishments/PunishType.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
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
    /** Captured at selection and re-verified at every step, so slot reuse can't hit the wrong
     *  player; the name is resolved fresh at display time rather than carried here stale. */
    VoltMod::PlayerRef Target;
    int DurationSec = 0;  // 0 = permanent; ignored for Kick/Warn
    std::string Reason;
};

/**
 * Start the punish wizard for @p pending's type: duration -> reason -> confirm for timed
 * punishments, reason -> confirm for kick/warn. Every step (and the final confirm)
 * re-validates the target and the admin's permission, then issues.
 */
void StartPunishFlow(AdminSystem::App& app, int adminSlot, PendingPunishment pending);

/** Quick Punish list of the configured templates the admin may use on this target. */
std::shared_ptr<VoltMod::Menu> BuildQuickPunishMenu(AdminSystem::App& app, int adminSlot, VoltMod::PlayerRef target);

/** True if at least one configured template is usable by @p adminSlot against @p targetSlot. */
bool AnyTemplateUsable(AdminSystem::App& app, int adminSlot, VoltMod::PlayerRef target);

/** True if @p adminSlot may punish @p target with @p type's permission. The one answer the menu
 *  row and the flow's re-validation both ask, so a row cannot offer what the press will refuse. */
bool CanStillPunish(AdminSystem::App& app, int adminSlot, VoltMod::PlayerRef target, Punishments::PunishType type);

}  // namespace AdminSystem::Admin::Menu
