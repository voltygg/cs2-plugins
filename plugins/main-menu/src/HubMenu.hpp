#pragma once

#include "Config.hpp"

#include <Contracts/IMenuSection.hpp>
#include <VoltMod/App/ServiceExchange.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Engine/ConVars/ConVars.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <VoltMod/Messaging/Messages.hpp>
#include <memory>
#include <string>
#include <string_view>

namespace MainMenu
{

/** Builds the main menu from the configured tabs: one sidebar tab each, one row per entry. */
class HubMenu
{
public:
    HubMenu(const ConfigManager& config, VoltMod::Translations& translations, VoltMod::Messages& messages,
            VoltMod::ConVars& conVars, VoltMod::ServiceExchange& exchange)
        : _config(config), _translations(translations), _messages(messages), _conVars(conVars), _exchange(exchange)
    {}

    [[nodiscard]] std::shared_ptr<VoltMod::Menu> Build(int slot);

private:
    [[nodiscard]] std::shared_ptr<VoltMod::Menu> BuildTab(const Tab& tab, int slot);
    [[nodiscard]] VoltMod::MenuItem Row(const Entry& entry);
    void Run(const Entry& entry, int slot, VoltMod::MenuSurface& surface);
    [[nodiscard]] bool IsVisible(const Entry& entry, int slot);
    /** Asked each time, never kept: the publishing plugin can unload between calls. */
    [[nodiscard]] Contracts::IMenuSection* Section(std::string_view id);
    [[nodiscard]] std::string Text(int slot, std::string_view label) const;

    const ConfigManager& _config;
    VoltMod::Translations& _translations;
    VoltMod::Messages& _messages;
    VoltMod::ConVars& _conVars;
    VoltMod::ServiceExchange& _exchange;
};

}  // namespace MainMenu
