#pragma once

#include "PendingCheck.hpp"

namespace AdminSystem::Admin::CheatCheck::View
{

/** Send only the persistent center-HTML panel to the suspect. */
void RenderPanel(int slot, const PendingCheck& pc);

/** Send the chat instructions followed by the center-HTML panel. */
void Render(int slot, const PendingCheck& pc);

}  // namespace AdminSystem::Admin::CheatCheck::View
