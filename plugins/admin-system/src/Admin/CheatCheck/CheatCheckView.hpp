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

namespace AdminSystem::Admin::CheatCheck
{

/** Renders a pending check to its suspect. The referenced services must outlive the view. */
class CheatCheckView
{
public:
    CheatCheckView(VoltMod::Runtime& runtime, const Core::ConfigManager& config, Core::ChatService& chat)
        : _rt(runtime), _config(config), _chat(chat)
    {}

    /** Send only the persistent center-HTML panel to the suspect. */
    void RenderPanel(int slot, const PendingCheck& pc) const;

    /** Send the chat instructions followed by the center-HTML panel. */
    void Render(int slot, const PendingCheck& pc) const;

private:
    VoltMod::Runtime& _rt;
    const Core::ConfigManager& _config;
    Core::ChatService& _chat;
};

}  // namespace AdminSystem::Admin::CheatCheck
