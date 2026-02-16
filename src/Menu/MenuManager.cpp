#include "MenuManager.hpp"
#include "MenuRenderer.hpp"
#include "../Sdk/Entity.hpp"
#include "../Sdk/UserMessage.hpp"
#include "../Utils/Log.hpp"

#include <ISmmPlugin.h>
#include <chrono>

using namespace AdminSystem::Utils;

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

namespace AdminSystem::Menu {

using namespace AdminSystem::Sdk;

static int64_t GetCurrentTimeMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

void MenuManager::OpenMenu(int slot, std::shared_ptr<Menu> menu)
{
    if (slot < 0 || slot >= 64 || !menu)
        return;

    auto& state = _states[slot];
    state.MenuStack.push(std::move(menu));
    state.SelectedIndex = 0;
    state.LastInputTime = GetCurrentTimeMs();

    Log::Info("Menu opened for slot {} (title: {}, items: {})",
                     slot, state.GetCurrentMenu()->Title, state.GetCurrentMenu()->Items.size());
}

void MenuManager::CloseMenu(int slot)
{
    if (slot < 0 || slot >= 64)
        return;

    auto& state = _states[slot];
    if (state.MenuStack.empty())
        return;

    auto menu = state.MenuStack.top();
    state.MenuStack.pop();

    // Fire close callback
    if (menu->OnClose)
        menu->OnClose(slot);

    if (state.MenuStack.empty())
    {
        // No more menus - clear the HUD
        ClearCenterHtml(slot);
        state.Reset();
    }
    else
    {
        // Returned to parent menu - reset selection
        state.SelectedIndex = 0;
    }
}

void MenuManager::CloseAllMenus(int slot)
{
    if (slot < 0 || slot >= 64)
        return;

    auto& state = _states[slot];
    state.Reset();
    ClearCenterHtml(slot);
}

bool MenuManager::HasActiveMenu(int slot) const
{
    if (slot < 0 || slot >= 64)
        return false;

    return _states[slot].HasMenu();
}

void MenuManager::OnGameFrame()
{
    for (int slot = 0; slot < 64; ++slot)
    {
        auto& state = _states[slot];
        if (!state.HasMenu())
            continue;

        // Read current button state
        uint64_t buttons = GetPlayerButtons(slot);
        uint64_t prev = state.PrevButtons;
        state.PrevButtons = buttons;

        // Handle input (edge-detected: newly pressed this frame)
        HandleInput(slot, buttons, prev);
        RenderMenu(slot);
    }
}

void MenuManager::HandleInput(int slot, uint64_t buttons, uint64_t prevButtons)
{
    auto& state = _states[slot];
    auto* menu = state.GetCurrentMenu();
    if (!menu)
        return;

    // Edge detection: button pressed this frame but not last frame
    uint64_t pressed = buttons & ~prevButtons;
    if (pressed == 0)
        return;

    // Debounce
    int64_t now = GetCurrentTimeMs();
    if (now - state.LastInputTime < InputDebounceMs)
        return;

    int itemCount = static_cast<int>(menu->Items.size());
    if (itemCount == 0)
        return;

    bool inputHandled = false;

    if (pressed & IN_FORWARD)
    {
        // Move selection up, wrapping around
        state.SelectedIndex = (state.SelectedIndex - 1 + itemCount) % itemCount;

        // Skip disabled items (up direction)
        int attempts = itemCount;
        while (!menu->Items[state.SelectedIndex].Enabled && --attempts > 0)
            state.SelectedIndex = (state.SelectedIndex - 1 + itemCount) % itemCount;

        inputHandled = true;
    }
    else if (pressed & IN_BACK)
    {
        // Move selection down, wrapping around
        state.SelectedIndex = (state.SelectedIndex + 1) % itemCount;

        // Skip disabled items (down direction)
        int attempts = itemCount;
        while (!menu->Items[state.SelectedIndex].Enabled && --attempts > 0)
            state.SelectedIndex = (state.SelectedIndex + 1) % itemCount;

        inputHandled = true;
    }
    else if (pressed & IN_USE)
    {
        // Execute selected item
        if (state.SelectedIndex >= 0 && state.SelectedIndex < itemCount)
        {
            const auto& item = menu->Items[state.SelectedIndex];
            if (item.Enabled && item.OnSelect)
                item.OnSelect(slot);
        }
        inputHandled = true;
    }
    else if (pressed & IN_RELOAD)
    {
        // Go back / close menu - no mutex, so we can call CloseMenu directly
        CloseMenu(slot);
        inputHandled = true;
    }

    if (inputHandled)
    {
        state.LastInputTime = now;
    }
}

void MenuManager::RenderMenu(int slot)
{
    auto& state = _states[slot];
    auto* menu = state.GetCurrentMenu();
    if (!menu)
        return;

    std::string html = RenderMenuHtml(menu, state.SelectedIndex);
    SendCenterHtml(slot, html);
}

void MenuManager::OnPlayerDisconnect(int slot)
{
    if (slot < 0 || slot >= 64)
        return;

    _states[slot].Reset();
}

} // namespace AdminSystem::Menu
