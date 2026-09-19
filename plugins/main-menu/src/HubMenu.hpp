#pragma once

#include "Config.hpp"

#include <Contracts/IMenuSection.hpp>
#include <VoltMod/App/ServiceExchange.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Engine/ConVars/ConVars.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <VoltMod/Menu/MenuRouter.hpp>
#include <VoltMod/Messaging/Messages.hpp>
#include <memory>
#include <string>
#include <string_view>

namespace MainMenu
{

/** The main menu: one tab per configured tab, then Settings. */
class HubMenu
{
public:
    HubMenu(const ConfigManager& config, VoltMod::Translations& translations, VoltMod::Messages& messages,
            VoltMod::ConVars& conVars, VoltMod::ServiceExchange& exchange, VoltMod::MenuRouter& menus)
        : _config(config),
          _translations(translations),
          _messages(messages),
          _conVars(conVars),
          _exchange(exchange),
          _menus(menus)
    {}

    [[nodiscard]] std::shared_ptr<VoltMod::Menu> Build(int slot);
    void Open(int slot);

    /** Opens the section another plugin publishes as @p id in place of this menu, or tells the player it is
     * unavailable. */
    void OpenSection(std::string_view id, int slot, VoltMod::MenuSurface& surface);

private:
    [[nodiscard]] std::shared_ptr<VoltMod::Menu> BuildTab(const Tab& tab, int slot);
    [[nodiscard]] std::shared_ptr<VoltMod::Menu> BuildSettings(int slot);
    /** Applies the pick for every plugin until the player leaves, then reopens the menu in it. */
    void SetLanguage(int slot, const std::string& lang);
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
    VoltMod::MenuRouter& _menus;
};

}  // namespace MainMenu
