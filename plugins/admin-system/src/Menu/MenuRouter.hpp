#pragma once

#include "Menu/PanoramaMenu.hpp"

#include <VoltMod/Menu/CenterHtmlMenu.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace AdminSystem::Menus
{

/**
 * @brief The one menu surface the plugin opens menus on and row callbacks reach.
 *
 * A session starts on the Panorama screen when the player can see it and on center text otherwise;
 * every later call follows the surface that holds that player's session. Never spawns anything to decide.
 */
class MenuRouter final : public VoltMod::MenuSurface
{
public:
    /** Both must outlive this. */
    MenuRouter(PanoramaMenu& panorama, VoltMod::CenterHtmlMenu& centerText);

    /** Start a session for @p slot, closing any session the player already has on either surface. */
    void Open(int slot, std::shared_ptr<VoltMod::Menu> menu, VoltMod::MenuOptions options);

    /** Push @p menu onto the player's session, starting one with default options if none is open. */
    void Open(int slot, std::shared_ptr<VoltMod::Menu> menu) override;
    void Close(int slot) override;
    void CloseAll(int slot) override;
    void CloseAll(int slot, std::string_view replyKey) override;
    void Prompt(int slot, std::string prompt, std::function<bool(int slot, std::string_view text)> callback) override;
    [[nodiscard]] std::string Translate(int slot, std::string_view key, std::string_view fallback) const override;

private:
    /** The surface holding @p slot's session; center text when there is none. */
    [[nodiscard]] VoltMod::MenuSurface& SessionOf(int slot) const;

    PanoramaMenu& _panorama;
    VoltMod::CenterHtmlMenu& _centerText;
};

}  // namespace AdminSystem::Menus
