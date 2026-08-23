#pragma once

#include "PendingCheck.hpp"

namespace CS2Kit
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
void RenderPanel(CS2Kit::Runtime& rt, const Core::ConfigManager& config, int slot, const PendingCheck& pc);

/** Send the chat instructions followed by the center-HTML panel. */
void Render(CS2Kit::Runtime& rt, const Core::ConfigManager& config, Core::ChatService& chat, int slot,
            const PendingCheck& pc);

}  // namespace AdminSystem::Admin::CheatCheck::View
