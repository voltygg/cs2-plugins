#include "PanoramaMenu.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Slot.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

using VoltMod::Capability;
using VoltMod::Menu;
using VoltMod::UiClick;
using VoltMod::UiPanel;

namespace AdminSystem::Menus
{

/** What a Button on the screen does when pressed. */
enum class Click
{
    Cancel,
    Back,
    Close,
    PagePrev,
    PageNext,
    Tab,
    Press,
    StepDown,
    StepUp,
};

/** One Button id and what it stands for; @ref Index is the tab or row for the pooled ones. */
struct ClickTarget
{
    std::string_view Id;
    Click Kind;
    int Index = 0;
};

/** Every Button the layout ships, built once from the generated ids. */
static constexpr auto ClickTargets = [] {
    namespace Screen = AdminUi::Menu;
    std::array<ClickTarget, 5 + TabCount + 3 * RowsPerPage> targets{};
    std::size_t n = 0;
    targets[n++] = {Screen::Cancel, Click::Cancel};
    targets[n++] = {Screen::Back, Click::Back};
    targets[n++] = {Screen::Close, Click::Close};
    targets[n++] = {Screen::PagePrev, Click::PagePrev};
    targets[n++] = {Screen::PageNext, Click::PageNext};
    for (int tab = 0; tab < TabCount; ++tab)
        targets[n++] = {Screen::Tabs[static_cast<std::size_t>(tab)].Id, Click::Tab, tab};
    for (int row = 0; row < RowsPerPage; ++row)
    {
        const Screen::Row& ids = Screen::Rows[static_cast<std::size_t>(row)];
        targets[n++] = {ids.Btn, Click::Press, row};
        targets[n++] = {ids.Dec, Click::StepDown, row};
        targets[n++] = {ids.Inc, Click::StepUp, row};
    }
    return targets;
}();

PanoramaMenu::PanoramaMenu(VoltMod::Runtime& runtime)
    : _rt(runtime), _stack(*this, runtime.Translations, runtime.Scheduler)
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

    // The addon is how the layout reaches players who did not compile it into their own client.
    // Without one nothing is required, so a hand-compiled client is drawn to and everyone else
    // sees an empty panel: said out loud here rather than left as a silent blank menu.
    if (addonId != 0)
    {
        if (auto required = _rt.Addons.Require(addonId))
            _addon = std::move(*required);
        else
            VoltMod::Log::Warn("Admin menu: addon {} not required ({}); clients without the layout "
                               "will see nothing.",
                               addonId, required.error().Detail);
    }
    else
    {
        VoltMod::Log::Warn("Admin menu: Panorama with no addon required. Only a client you "
                           "compiled the layout into can see it.");
    }

    _enabled = true;
    _subs.Add(_rt.Ui.Clicked += [this](const UiClick& click) { OnClick(click); });

    // A held commit lands on a timer rather than on a press, and this surface only draws when
    // something asks it to.
    _subs.Add(_stack.Committed += [this](int slot) {
        if (IsOpen(slot))
            Draw(slot);
    });
}

bool PanoramaMenu::CanDraw(int slot)
{
    if (!_enabled || !VoltMod::IsValidSlot(slot))
        return false;

    for (Capability capability : {Capability::CustomUi, Capability::UiClicks})
    {
        if (!_rt.Capabilities.Has(capability))
            return false;
    }

    // A client still fetching the addon has no layout to draw the panel on yet.
    if (_rt.Addons.HasPending(slot))
        return false;

    UiPanel& panel = _screen.Panel(slot);
    return panel && panel.Prepare(slot);
}

bool PanoramaMenu::Open(int slot, std::shared_ptr<Menu> menu, VoltMod::MenuOptions options)
{
    if (!menu || !CanDraw(slot))
        return false;

    CloseAll(slot);
    _stack.Push(slot, std::move(menu));

    Session& session = _sessions[slot];
    session.Page = 0;
    session.SelectedTab = -1;
    session.Tabs.clear();

    // The tab strip stands for the root menu's submenus, so it is read once per session: the
    // labels come back with the kinds rather than costing a Describe per tab per draw.
    const Menu* root = _stack.Root(slot);
    for (int index = 0; root && index < static_cast<int>(root->Items.size()); ++index)
    {
        if (static_cast<int>(session.Tabs.size()) >= TabCount)
            break;
        if (VoltMod::MenuRow described = _stack.Describe(slot, index);
            described.Kind == VoltMod::MenuRowKind::Submenu)
            session.Tabs.push_back({.RootIndex = index, .Label = std::move(described.Label)});
    }

    _rt.Freeze.Open(slot, options.FreezeMovement);

    _screen.Show(slot, /*capture=*/true);
    Draw(slot);
    return true;
}

bool PanoramaMenu::IsOpen(int slot) const
{
    return _stack.IsOpen(slot);
}

int PanoramaMenu::ItemAt(int slot, int row) const
{
    return _sessions[slot].Page * RowsPerPage + row;
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
    // Reply before closing: it is addressed to a player whose menus are about to go.
    if (auto& reply = _rt.Policy.Reply; reply)
        reply(slot, _rt.Translations.Get(std::string(replyKey), slot));

    CloseAll(slot);
}

void PanoramaMenu::Prompt(int slot, std::string prompt, std::function<bool(int, std::string_view)> callback)
{
    _rt.Hooks.ChatInput.BeginCapture(slot, std::move(prompt), std::move(callback));
    DrawPrompt(VoltMod::UiPanelWriter{_screen.Panel(slot), slot});
}

std::string PanoramaMenu::Translate(int slot, std::string_view key, std::string_view fallback) const
{
    return _rt.Translations.GetOr(key, slot, fallback);
}

void PanoramaMenu::OnClick(const UiClick& click)
{
    const int slot = click.Slot;
    if (!IsOpen(slot))
        return;

    const auto target = std::ranges::find(ClickTargets, click.ButtonId, &ClickTarget::Id);
    if (target == ClickTargets.end())
        return;

    // A prompt owns every press but its own Cancel: answering it is a chat line, not a click.
    if (target->Kind == Click::Cancel)
    {
        _rt.Hooks.ChatInput.CancelCapture(slot);
        Draw(slot);
        return;
    }
    if (_rt.Hooks.ChatInput.IsCapturing(slot))
        return;

    switch (target->Kind)
    {
    case Click::Back:
        return Close(slot);
    case Click::Close:
        return CloseAll(slot);
    case Click::PagePrev:
        return TurnPage(slot, -1);
    case Click::PageNext:
        return TurnPage(slot, +1);
    case Click::Tab:
        return OpenTab(slot, target->Index);
    case Click::Press:
        return Activate(slot, ItemAt(slot, target->Index));
    case Click::StepDown:
        return StepRow(slot, target->Index, -1);
    case Click::StepUp:
        return StepRow(slot, target->Index, +1);
    case Click::Cancel:
        break;
    }
}

void PanoramaMenu::Activate(int slot, int index)
{
    // Entering a branch from the root records which tab it belongs to, so the strip stays lit.
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
    const int pages = VoltMod::PageCount(static_cast<int>(menu->Items.size()), RowsPerPage);
    session.Page = VoltMod::WrapIndex(session.Page + delta, pages);
    Draw(slot);
}

void PanoramaMenu::Hide(int slot)
{
    Session& session = _sessions[slot];
    session.Tabs.clear();
    session.SelectedTab = -1;
    session.Page = 0;

    _rt.Hooks.ChatInput.CancelCapture(slot);
    _rt.Freeze.Close(slot);
    _screen.Hide(slot);
}

}  // namespace AdminSystem::Menus
