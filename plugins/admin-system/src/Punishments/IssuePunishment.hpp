#pragma once

#include "../Core/App.hpp"
#include "PunishType.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Players/Player.hpp>
#include <cstdint>
#include <string>

namespace AdminSystem::Punishments
{

/**
 * Fill and issue a punishment of the given type against an online target - the single entry
 * point shared by the chat commands and the admin menu. Kick has no DB row: the target is
 * kicked and the action broadcast directly. Returns false only when persisting to the DB failed.
 */
bool IssuePunishment(App& app, const VoltMod::Player& admin, const VoltMod::Player& target, PunishType type,
                     const std::string& reason, int64_t durationSec);

}  // namespace AdminSystem::Punishments
