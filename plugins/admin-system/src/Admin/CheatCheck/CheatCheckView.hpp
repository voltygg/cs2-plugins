#pragma once

#include "../../Core/ChatService.hpp"
#include "../../Core/Config.hpp"
#include "PendingCheck.hpp"

#include <VoltMod/Runtime.hpp>
#include <string>

namespace AdminSystem::Admin::CheatCheck
{

/** Renders a pending check to its suspect. The referenced services must outlive the view. */
class CheatCheckView
{
public:
    CheatCheckView(VoltMod::Runtime& runtime, const Core::ConfigManager& config, Core::ChatService& chat)
        : _rt(runtime), _config(config), _chat(chat)
    {}

    /** Build the center-HTML panel for @p slot. Pure: the caller decides when to send it, which
     *  is what lets Runtime::CenterHtml own the re-send loop. */
    std::string PanelHtml(int slot, const PendingCheck& pc) const;

    /** Send the one-off chat instructions. The panel is on its own refresh loop. */
    void SendInstructions(int slot, const PendingCheck& pc) const;

private:
    VoltMod::Runtime& _rt;
    const Core::ConfigManager& _config;
    Core::ChatService& _chat;
};

}  // namespace AdminSystem::Admin::CheatCheck
