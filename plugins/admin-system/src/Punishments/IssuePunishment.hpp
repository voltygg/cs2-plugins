#pragma once

#include "../Core/App.hpp"
#include "PunishType.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Players/Player.hpp>
#include <cstdint>
#include <string>

namespace AdminSystem::Punishments
{

/** Issue a punishment for an online target from chat commands or the admin menu, and audit it
 *  against the admin. Kick has no DB row and is broadcast directly; the other kinds persist async. */
void IssuePunishment(App& app, const VoltMod::Player& admin, const VoltMod::Player& target, PunishType type,
                     const std::string& reason, int64_t durationSec);

}  // namespace AdminSystem::Punishments
