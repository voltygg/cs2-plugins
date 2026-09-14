#include "Menu/PanoramaMenu.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Slot.hpp>
#include <algorithm>
#include <cstddef>
#include <format>
#include <utility>

using VoltMod::Capability;
using VoltMod::Menu;

namespace AdminSystem::Menus
{

PanoramaMenu::PanoramaMenu(VoltMod::Runtime& runtime)
    : _rt(runtime), _stack(*this, runtime.Translations, runtime.Scheduler), _screen(runtime.Screens, runtime.Slots)
{}

PanoramaMenu::~PanoramaMenu() = default;

void PanoramaMenu::Start(bool enabled, uint64_t addonId)
{
    _stack.BindReset(_rt.Slots);
    _sessions.BindReset(_rt.Slots);

    if (!enabled)
    {
        VoltMod::Log::Info("Admin menu: center HTML, as configured.");
        return;
    }

    // Without an addon only a hand-compiled client has the layout; say so rather than draw blanks.
    if (addonId != 0)
    {
        if (auto required = _rt.Addons.Require(addonId))
            _addon = std::move(*required);
        else
            VoltMod::Log::Warn("Admin menu: addon {} not required ({}); clients without the layout will see nothing.",
                               addonId, required.error().Detail);
    }
    else
    {
        VoltMod::Log::Warn("Admin menu: Panorama with no addon required. Only a client you compiled the layout "
                           "into can see it.");
    }

    _enabled = true;
    _subs.Add(_rt.Screens.Pressed += [this](const VoltMod::ButtonPress& press) { OnPress(press); });

    // A held commit lands on a timer, not a press, so nothing else would redraw it.
    _subs.Add(_stack.Committed += [this](int slot) {
        if (IsOpen(slot))
            Draw(slot);
    });
}

bool PanoramaMenu::Open(int slot, std::shared_ptr<Menu> menu, VoltMod::MenuOptions options)
{
    if (!menu || !CanUse(slot))
        return false;

    CloseAll(slot);
    if (!_screen.Show(slot))
        return false;

    _stack.Push(slot, std::move(menu));
    ReadTabs(slot);
    _rt.Freeze.Open(slot, options.FreezeMovement);
    Draw(slot);
    return true;
}

bool PanoramaMenu::IsOpen(int slot) const
{
    return _stack.IsOpen(slot);
}

void PanoramaMenu::Open(int slot, std::shared_ptr<Menu> menu)
{
    if (!menu)
        return;
    if (!IsOpen(slot))
    {
        Open(slot, std::move(menu), {});
        return;
    }

    _stack.Push(slot, std::move(menu));
    _sessions[slot].Page = 0;
    Draw(slot);
}

void PanoramaMenu::Close(int slot)
{
    if (!IsOpen(slot))
        return;

    _rt.Hooks.ChatInput.CancelCapture(slot);

    _stack.Pop(slot);
    if (!IsOpen(slot))
    {
        Hide(slot);
        return;
    }

    Session& session = _sessions[slot];
    session.Page = 0;
    if (_stack.Depth(slot) <= 1)
        session.SelectedTab = -1;
    Draw(slot);
}

void PanoramaMenu::CloseAll(int slot)
{
    if (!IsOpen(slot))
        return;

    _stack.Clear(slot);
    Hide(slot);
}

void PanoramaMenu::CloseAll(int slot, std::string_view replyKey)
{
    // Reply first: it is addressed to a player whose menus are about to go.
    if (auto& reply = _rt.Policy.Reply; reply)
        reply(slot, _rt.Translations.Get(std::string(replyKey), slot));

    CloseAll(slot);
}

void PanoramaMenu::Prompt(int slot, std::string prompt, std::function<bool(int, std::string_view)> callback)
{
    _rt.Hooks.ChatInput.BeginCapture(slot, std::move(prompt), std::move(callback));
    DrawPrompt(slot);
}

std::string PanoramaMenu::Translate(int slot, std::string_view key, std::string_view fallback) const
{
    return _rt.Translations.GetOr(key, slot, fallback);
}

bool PanoramaMenu::CanUse(int slot) const
{
    if (!_enabled || !VoltMod::IsValidSlot(slot))
        return false;

    for (Capability capability : {Capability::CustomUi, Capability::UiClicks})
    {
        if (!_rt.Capabilities.Has(capability))
            return false;
    }

    // A client still fetching the addon has no layout to draw on yet.
    return !_rt.Addons.HasMissing(slot);
}

int PanoramaMenu::ItemAt(int slot, int row) const
{
    return _sessions[slot].Page * AdminMenuScreen::RowCount + row;
}

void PanoramaMenu::ReadTabs(int slot)
{
    Session& session = _sessions[slot];
    session = {};

    // Tabs are the root's submenus, read once per session rather than described on every draw.
    const Menu* root = _stack.Root(slot);
    for (int index = 0; root && index < static_cast<int>(root->Items.size()); ++index)
    {
        if (static_cast<int>(session.Tabs.size()) >= AdminMenuScreen::TabCount)
            break;
        if (VoltMod::MenuRow described = _stack.Describe(slot, index); described.Kind == VoltMod::MenuRowKind::Submenu)
            session.Tabs.push_back(
                {.RootIndex = index, .Label = std::move(described.Label), .Icon = std::move(described.Icon)});
    }
}

void PanoramaMenu::Draw(int slot)
{
    const Menu* menu = _stack.Current(slot);
    if (!menu)
        return;

    // Also what brings the screen back after a map change removed it; a player it cannot reach loses the session.
    if (!_screen.Show(slot))
    {
        CloseAll(slot);
        return;
    }

    DrawHeader(slot, *menu);
    DrawTabs(slot);
    DrawRows(slot, *menu);
    DrawPrompt(slot);

    const bool deep = _stack.Depth(slot) > 1;
    _screen.SetFooter(slot, Translate(slot, deep ? "nav.back" : "nav.root", deep ? "Back" : "Main"),
                      Translate(slot, "menu.cancel", "Cancel"));
}

void PanoramaMenu::DrawHeader(int slot, const Menu& menu)
{
    const Menu* root = _stack.Root(slot);
    const Menu& brand = root ? *root : menu;

    // The root's subtitle already sits under the brand.
    _screen.SetHeader(slot, {.Brand = brand.Title,
                             .BrandSubtitle = brand.Subtitle,
                             .Breadcrumb = _stack.Breadcrumb(slot),
                             .Title = menu.Title,
                             .Subtitle = &menu != &brand ? std::string_view(menu.Subtitle) : std::string_view{}});
}

void PanoramaMenu::DrawTabs(int slot)
{
    const Session& session = _sessions[slot];

    // Without tabs the sidebar would hold only a copy of the title.
    _screen.SetSidebarVisible(slot, !session.Tabs.empty());

    for (int tab = 0; tab < AdminMenuScreen::TabCount; ++tab)
    {
        if (tab >= static_cast<int>(session.Tabs.size()))
        {
            _screen.HideTab(slot, tab);
            continue;
        }

        const Tab& shown = session.Tabs[static_cast<std::size_t>(tab)];
        _screen.SetTab(slot, tab, shown.Label, shown.Icon, tab == session.SelectedTab);
    }
}

void PanoramaMenu::DrawRows(int slot, const Menu& menu)
{
    Session& session = _sessions[slot];

    const int count = static_cast<int>(menu.Items.size());
    const int pages = VoltMod::PageCount(count, AdminMenuScreen::RowCount);
    session.Page = std::clamp(session.Page, 0, pages - 1);

    const std::string pendingHint = Translate(slot, "menu.pending", "Applying...");
    for (int row = 0; row < AdminMenuScreen::RowCount; ++row)
    {
        const int item = ItemAt(slot, row);
        if (item >= count)
            _screen.HideRow(slot, row);
        else
            _screen.SetRow(slot, row, _stack.Describe(slot, item), pendingHint);
    }

    if (count == 0)
        _screen.ShowEmpty(slot, Translate(slot, "menu.empty", "Nothing here"));
    else
        _screen.HideEmpty(slot);

    if (pages > 1)
        _screen.ShowPager(slot, std::format("{} / {}", session.Page + 1, pages));
    else
        _screen.HidePager(slot);
}

void PanoramaMenu::DrawPrompt(int slot)
{
    if (const auto prompt = _rt.Hooks.ChatInput.GetPrompt(slot))
        _screen.ShowPrompt(slot, *prompt, Translate(slot, "menu.promptHint", "Type your answer in chat"));
    else
        _screen.HidePrompt(slot);
}

void PanoramaMenu::OnPress(const VoltMod::ButtonPress& press)
{
    const int slot = press.Slot;
    if (!IsOpen(slot))
        return;

    const auto button = AdminMenuScreen::ButtonFor(press.ButtonId);
    if (!button)
        return;

    // A prompt owns every press but its own Cancel: answering it is a chat line, not a click.
    if (button->Kind == MenuButtonKind::Cancel)
    {
        _rt.Hooks.ChatInput.CancelCapture(slot);
        Draw(slot);
        return;
    }
    if (_rt.Hooks.ChatInput.IsCapturing(slot))
        return;

    switch (button->Kind)
    {
    case MenuButtonKind::Back:
        return Close(slot);
    case MenuButtonKind::Close:
        return CloseAll(slot);
    case MenuButtonKind::PreviousPage:
        return TurnPage(slot, -1);
    case MenuButtonKind::NextPage:
        return TurnPage(slot, +1);
    case MenuButtonKind::Tab:
        return OpenTab(slot, button->Index);
    case MenuButtonKind::Row:
        return Activate(slot, ItemAt(slot, button->Index));
    case MenuButtonKind::StepDown:
        return StepRow(slot, button->Index, -1);
    case MenuButtonKind::StepUp:
        return StepRow(slot, button->Index, +1);
    case MenuButtonKind::Cancel:
        break;
    }
}

void PanoramaMenu::Activate(int slot, int index)
{
    // Entering a branch from the root records which tab it belongs to, so the tab stays lit.
    Session& session = _sessions[slot];
    if (_stack.Depth(slot) == 1)
    {
        const auto found = std::ranges::find(session.Tabs, index, &Tab::RootIndex);
        session.SelectedTab = found == session.Tabs.end() ? -1 : static_cast<int>(found - session.Tabs.begin());
    }

    _stack.Activate(slot, index);

    // Activation may have opened, replaced or closed the session; only redraw one still here.
    if (IsOpen(slot))
        Draw(slot);
}

void PanoramaMenu::StepRow(int slot, int row, int direction)
{
    if (_stack.Step(slot, ItemAt(slot, row), direction))
        Draw(slot);
}

void PanoramaMenu::OpenTab(int slot, int tab)
{
    Session& session = _sessions[slot];
    if (tab < 0 || tab >= static_cast<int>(session.Tabs.size()))
        return;

    // A tab is a jump, not a push: back to the root before entering the branch it stands for.
    _stack.PopToRoot(slot);
    session.Page = 0;
    Activate(slot, session.Tabs[static_cast<std::size_t>(tab)].RootIndex);
}

void PanoramaMenu::TurnPage(int slot, int delta)
{
    const Menu* menu = _stack.Current(slot);
    if (!menu)
        return;

    _stack.ApplyPending(slot);
    Session& session = _sessions[slot];
    const int pages = VoltMod::PageCount(static_cast<int>(menu->Items.size()), AdminMenuScreen::RowCount);
    session.Page = VoltMod::WrapIndex(session.Page + delta, pages);
    Draw(slot);
}

void PanoramaMenu::Hide(int slot)
{
    _sessions[slot] = {};
    _rt.Hooks.ChatInput.CancelCapture(slot);
    _rt.Freeze.Close(slot);
    _screen.Hide(slot);
}

}  // namespace AdminSystem::Menus
