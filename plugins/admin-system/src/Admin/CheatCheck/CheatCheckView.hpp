#pragma once

#include "PendingCheck.hpp"

namespace AdminSystem
{
struct App;
}

namespace AdminSystem::Admin::CheatCheck::View
{

/** Send only the persistent center-HTML panel to the suspect. */
void RenderPanel(App& app, int slot, const PendingCheck& pc);

/** Send the chat instructions followed by the center-HTML panel. */
void Render(App& app, int slot, const PendingCheck& pc);

}  // namespace AdminSystem::Admin::CheatCheck::View
