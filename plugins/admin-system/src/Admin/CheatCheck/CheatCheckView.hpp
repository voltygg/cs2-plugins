#pragma once

#include "PendingCheck.hpp"

namespace VoltMod
{
class Runtime;
}

namespace AdminSystem::Core
{
class ChatService;
class ConfigManager;
}  // namespace AdminSystem::Core

namespace AdminSystem::Admin::CheatCheck::View
{

/** Send only the persistent center-HTML panel to the suspect. */
void RenderPanel(VoltMod::Runtime& rt, const Core::ConfigManager& config, int slot, const PendingCheck& pc);

/** Send the chat instructions followed by the center-HTML panel. */
void Render(VoltMod::Runtime& rt, const Core::ConfigManager& config, Core::ChatService& chat, int slot,
            const PendingCheck& pc);

}  // namespace AdminSystem::Admin::CheatCheck::View
