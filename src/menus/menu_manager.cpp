#include "menu_manager.h"
#include "menu_renderer.h"
#include "../sdk/entity.h"
#include "../sdk/usermessage.h"
#include <ISmmPlugin.h>

#include <chrono>

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

namespace menus {

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

    std::lock_guard<std::mutex> lock(m_mutex);

    auto& state = m_states[slot];
    state.menu_stack.push(std::move(menu));
    state.selected_index = 0;
    state.last_input_time = GetCurrentTimeMs();
    state.last_render_time = 0; // Force immediate render
}

void MenuManager::CloseMenu(int slot)
{
    if (slot < 0 || slot >= 64)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);

    auto& state = m_states[slot];
    if (state.menu_stack.empty())
        return;

    auto menu = state.menu_stack.top();
    state.menu_stack.pop();

    // Fire close callback
    if (menu->on_close)
        menu->on_close(slot);

    if (state.menu_stack.empty())
    {
        // No more menus - clear the HUD
        sdk::ClearCenterHtml(slot);
        state.Reset();
    }
    else
    {
        // Returned to parent menu - reset selection
        state.selected_index = 0;
        state.last_render_time = 0; // Force re-render
    }
}

void MenuManager::CloseAllMenus(int slot)
{
    if (slot < 0 || slot >= 64)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);

    auto& state = m_states[slot];
    state.Reset();
    sdk::ClearCenterHtml(slot);
}

bool MenuManager::HasActiveMenu(int slot) const
{
    if (slot < 0 || slot >= 64)
        return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_states[slot].HasMenu();
}

void MenuManager::OnGameFrame()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (int slot = 0; slot < 64; ++slot)
    {
        auto& state = m_states[slot];
        if (!state.HasMenu())
            continue;

        // Read current button state
        uint64_t buttons = sdk::GetPlayerButtons(slot);
        uint64_t prev = state.prev_buttons;
        state.prev_buttons = buttons;

        // Handle input (edge-detected: newly pressed this frame)
        HandleInput(slot, buttons, prev);

        // Re-render at interval (center HTML fades, needs refresh)
        int64_t now = GetCurrentTimeMs();
        if (now - state.last_render_time >= RENDER_INTERVAL_MS)
        {
            RenderMenu(slot);
            state.last_render_time = now;
        }
    }
}

void MenuManager::HandleInput(int slot, uint64_t buttons, uint64_t prev_buttons)
{
    auto& state = m_states[slot];
    auto* menu = state.GetCurrentMenu();
    if (!menu)
        return;

    // Edge detection: button pressed this frame but not last frame
    uint64_t pressed = buttons & ~prev_buttons;
    if (pressed == 0)
        return;

    // Debounce
    int64_t now = GetCurrentTimeMs();
    if (now - state.last_input_time < INPUT_DEBOUNCE_MS)
        return;

    int item_count = static_cast<int>(menu->items.size());
    if (item_count == 0)
        return;

    bool input_handled = false;

    if (pressed & sdk::IN_FORWARD)
    {
        // Move selection up, wrapping around
        state.selected_index = (state.selected_index - 1 + item_count) % item_count;

        // Skip disabled items (up direction)
        int attempts = item_count;
        while (!menu->items[state.selected_index].enabled && --attempts > 0)
            state.selected_index = (state.selected_index - 1 + item_count) % item_count;

        input_handled = true;
    }
    else if (pressed & sdk::IN_BACK)
    {
        // Move selection down, wrapping around
        state.selected_index = (state.selected_index + 1) % item_count;

        // Skip disabled items (down direction)
        int attempts = item_count;
        while (!menu->items[state.selected_index].enabled && --attempts > 0)
            state.selected_index = (state.selected_index + 1) % item_count;

        input_handled = true;
    }
    else if (pressed & sdk::IN_USE)
    {
        // Execute selected item
        if (state.selected_index >= 0 && state.selected_index < item_count)
        {
            const auto& item = menu->items[state.selected_index];
            if (item.enabled && item.on_select)
                item.on_select(slot);
        }
        input_handled = true;
    }
    else if (pressed & sdk::IN_RELOAD)
    {
        // Go back / close menu (release lock before calling CloseMenu)
        // We can't call CloseMenu here since it also locks m_mutex.
        // Instead, handle the pop inline.
        auto current = state.menu_stack.top();
        state.menu_stack.pop();

        if (current->on_close)
            current->on_close(slot);

        if (state.menu_stack.empty())
        {
            sdk::ClearCenterHtml(slot);
            state.Reset();
        }
        else
        {
            state.selected_index = 0;
            state.last_render_time = 0;
        }
        input_handled = true;
    }

    if (input_handled)
    {
        state.last_input_time = now;
        state.last_render_time = 0; // Force immediate re-render
    }
}

void MenuManager::RenderMenu(int slot)
{
    auto& state = m_states[slot];
    auto* menu = state.GetCurrentMenu();
    if (!menu)
        return;

    std::string html = RenderMenuHtml(menu, state.selected_index);
    sdk::SendCenterHtml(slot, html);
}

void MenuManager::OnPlayerDisconnect(int slot)
{
    if (slot < 0 || slot >= 64)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_states[slot].Reset();
}

MenuManager& MenuManager::Instance()
{
    static MenuManager instance;
    return instance;
}

} // namespace menus
