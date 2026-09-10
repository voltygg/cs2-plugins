#include "PanoramaMenu.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Slot.hpp>
#include <algorithm>
#include <utility>

using VoltMod::Capability;
using VoltMod::Menu;
using VoltMod::UiClick;
using VoltMod::UiPanel;

namespace AdminSystem::Menus
{

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

bool PanoramaMenu::Available(int slot)
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
    if (!menu || !Available(slot))
        return false;

    CloseAll(slot);
    _stack.Push(slot, std::move(menu));

    Session& session = _sessions[slot];
    session.Page = 0;
    session.OpenTab = -1;
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
            session.Tabs.push_back({.Item = index, .Label = std::move(described.Label)});
    }

    if (options.FreezeMovement)
        session.Freeze.Hold(_rt.Entities.PawnOf(slot));

    _screen.Show(slot, /*capture=*/true);
    Draw(slot);
    return true;
}

bool PanoramaMenu::IsOpen(int slot) const
{
    return _stack.IsOpen(slot);
}

int PanoramaMenu::ItemIndex(int slot, int row) const
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
        Dismiss(slot);
        return;
    }

    Session& session = _sessions[slot];
    session.Page = 0;
    if (_stack.Depth(slot) <= 1)
        session.OpenTab = -1;
    Draw(slot);
}

void PanoramaMenu::CloseAll(int slot)
{
    if (!IsOpen(slot))
        return;

    _stack.Clear(slot);
    Dismiss(slot);
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
    DrawPrompt(slot);
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

    // A prompt owns every press but its own Cancel: answering it is a chat line, not a click.
    const bool prompting = _rt.Hooks.ChatInput.IsCapturing(slot);
    if (click.ButtonId == AdminUi::Menu::Cancel)
    {
        _rt.Hooks.ChatInput.CancelCapture(slot);
        Draw(slot);
        return;
    }
    if (prompting)
        return;

    if (click.ButtonId == AdminUi::Menu::Back)
        return Close(slot);
    if (click.ButtonId == AdminUi::Menu::Close)
        return CloseAll(slot);
    if (click.ButtonId == AdminUi::Menu::PagePrev)
        return TurnPage(slot, -1);
    if (click.ButtonId == AdminUi::Menu::PageNext)
        return TurnPage(slot, +1);

    for (int tab = 0; tab < TabCount; ++tab)
    {
        if (click.ButtonId == AdminUi::Menu::Tabs[tab].Id)
            return OpenTab(slot, tab);
    }

    for (int row = 0; row < RowsPerPage; ++row)
    {
        if (click.ButtonId == AdminUi::Menu::Rows[row].Btn)
            return Activate(slot, ItemIndex(slot, row));
        if (click.ButtonId == AdminUi::Menu::Rows[row].Dec)
            return Nudge(slot, row, -1);
        if (click.ButtonId == AdminUi::Menu::Rows[row].Inc)
            return Nudge(slot, row, +1);
    }
}

void PanoramaMenu::Activate(int slot, int index)
{
    // Entering a branch from the root records which tab it belongs to, so the strip stays lit.
    Session& session = _sessions[slot];
    if (_stack.Depth(slot) == 1)
    {
        const auto found = std::find_if(session.Tabs.begin(), session.Tabs.end(),
                                        [index](const Tab& tab) { return tab.Item == index; });
        session.OpenTab = found == session.Tabs.end() ? -1 : static_cast<int>(found - session.Tabs.begin());
    }

    _stack.Activate(slot, index);

    // Activation may have opened, replaced or closed the session; only redraw one still here.
    if (IsOpen(slot))
        Draw(slot);
}

void PanoramaMenu::Nudge(int slot, int row, int direction)
{
    if (_stack.Step(slot, ItemIndex(slot, row), direction))
        Draw(slot);
}

void PanoramaMenu::OpenTab(int slot, int tab)
{
    Session& session = _sessions[slot];
    if (tab < 0 || tab >= static_cast<int>(session.Tabs.size()))
        return;

    // A tab is a jump, not a push: unwind to the root before entering the branch it stands for.
    _stack.PopToRoot(slot);
    session.Page = 0;
    Activate(slot, session.Tabs[tab].Item);
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

void PanoramaMenu::Dismiss(int slot)
{
    Session& session = _sessions[slot];
    session.Tabs.clear();
    session.OpenTab = -1;
    session.Page = 0;

    _rt.Hooks.ChatInput.CancelCapture(slot);
    session.Freeze.Release(_rt.Entities.PawnOf(slot));
    _screen.Hide(slot);
}

}  // namespace AdminSystem::Menus
