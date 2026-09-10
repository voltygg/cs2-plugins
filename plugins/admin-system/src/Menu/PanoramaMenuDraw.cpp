#include "PanoramaMenu.hpp"

#include <VoltMod/Core/Slot.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>

using VoltMod::ClassChoice;
using VoltMod::Menu;
using VoltMod::MenuRow;
using VoltMod::MenuRowKind;
using VoltMod::UiPanel;

namespace AdminSystem::Menus
{

// VoltMod::MenuRowKind, lowercase and in enumerator order, so casting a kind to a class is safe.
static_assert(AdminUi::Menu::KindNames == std::array<std::string_view, 6>{"text", "button", "submenu", "toggle",
                                                                         "choice", "input"},
              "admin_menu's Kind classes must match VoltMod::MenuRowKind");

/** The accent a row kind carries, as an index into `AdminUi::Menu::AccentClasses`, or none. */
static int AccentFor(const MenuRow& row)
{
    if (!row.Enabled)
        return ClassChoice::None;
    switch (row.Kind)
    {
    case MenuRowKind::Submenu:
        return static_cast<int>(AdminUi::Menu::Accent::Info);
    case MenuRowKind::Toggle:
        return static_cast<int>(row.State.value_or(false) ? AdminUi::Menu::Accent::Success
                                                          : AdminUi::Menu::Accent::Common);
    case MenuRowKind::Choice:
        return static_cast<int>(AdminUi::Menu::Accent::Warning);
    case MenuRowKind::Input:
        return static_cast<int>(AdminUi::Menu::Accent::Rare);
    case MenuRowKind::Text:
        return ClassChoice::None;
    case MenuRowKind::Button:
        break;
    }
    return static_cast<int>(AdminUi::Menu::Accent::Contraband);
}

void PanoramaMenu::Draw(int slot)
{
    const Menu* menu = _stack.Current(slot);
    UiPanel& panel = _screen.Panel(slot);
    if (!menu || !panel)
        return;

    const PanelWriter w{panel, slot};
    DrawHeader(w, *menu);
    DrawTabs(w);
    DrawRows(w, *menu);
    DrawPrompt(w);

    // The two footer buttons say what they do: a submenu goes back, a root menu closes.
    const bool deep = _stack.Depth(slot) > 1;
    w.Set(Shell.Back, Translate(slot, deep ? "nav.back" : "nav.root", deep ? "Back" : "Main"));
    w.Set(Shell.Close, Translate(slot, "nav.close", "Close"));
    w.Set(Shell.Cancel, Translate(slot, "menu.cancel", "Cancel"));
}

void PanoramaMenu::DrawHeader(const PanelWriter& w, const Menu& menu)
{
    w.Set(Shell.Title, menu.Title);
    w.Set(Shell.Breadcrumb, _stack.Breadcrumb(w.Slot()));
    w.Set(Shell.Subtitle, menu.Subtitle);
    w.Set(Shell.SubtitleHidden, menu.Subtitle.empty());
}

void PanoramaMenu::DrawTabs(const PanelWriter& w)
{
    const Session& session = _sessions[w.Slot()];

    for (int tab = 0; tab < TabCount; ++tab)
    {
        const TabWriters& ids = Tabs[static_cast<std::size_t>(tab)];
        const bool used = tab < static_cast<int>(session.Tabs.size());
        w.Set(ids.Hidden, !used);
        if (!used)
            continue;

        w.Set(ids.Label, session.Tabs[static_cast<std::size_t>(tab)].Label);
        w.Set(ids.Selected, tab == session.SelectedTab);
    }
}

void PanoramaMenu::DrawRows(const PanelWriter& w, const Menu& menu)
{
    const int slot = w.Slot();
    Session& session = _sessions[slot];

    const int count = static_cast<int>(menu.Items.size());
    const int pages = VoltMod::PageCount(count, RowsPerPage);
    session.Page = std::clamp(session.Page, 0, pages - 1);

    for (int row = 0; row < RowsPerPage; ++row)
    {
        const int item = ItemAt(slot, row);
        if (item >= count)
        {
            w.Set(Rows[static_cast<std::size_t>(row)].Hidden, true);
            continue;
        }
        DrawRow(w, row, _stack.Describe(slot, item));
    }

    w.Set(Shell.EmptyHidden, count != 0);
    if (count == 0)
        w.Set(Shell.Empty, Translate(slot, "menu.empty", "Nothing here"));

    w.Set(Shell.PagerHidden, pages <= 1);
    if (pages > 1)
        w.Set(Shell.Page, std::format("{} / {}", session.Page + 1, pages));
}

void PanoramaMenu::DrawRow(const PanelWriter& w, int row, const MenuRow& described)
{
    const RowWriters& ids = Rows[static_cast<std::size_t>(row)];

    w.Set(ids.Hidden, false);
    w.Set(ids.Label, described.Label);
    w.Set(ids.Value, described.Value);
    w.Set(ids.HasValue, !described.Value.empty());
    w.Set(ids.Disabled, !described.Enabled);
    w.Set(ids.On, described.State.value_or(false));
    w.Set(ids.Kind, static_cast<int>(described.Kind));
    w.Set(ids.Accent, AccentFor(described));

    // Steppers only where A/D would have done something, and only while the row is live.
    w.Set(ids.HasSteppers, described.Steppable && described.Enabled);

    // The hint line doubles as the "not applied yet" note, which is also what Changed pulses on.
    w.Set(ids.HasHint, described.Pending);
    w.Set(ids.Changed, described.Pending);
    if (described.Pending)
        w.Set(ids.Hint, Translate(w.Slot(), "menu.pending", "Applying..."));
}

void PanoramaMenu::DrawPrompt(const PanelWriter& w)
{
    const auto prompt = _rt.Hooks.ChatInput.GetPrompt(w.Slot());

    w.Set(Shell.PromptHidden, !prompt.has_value());
    w.Set(Shell.Prompting, prompt.has_value());
    if (!prompt)
        return;

    w.Set(Shell.PromptText, *prompt);
    w.Set(Shell.PromptHint, Translate(w.Slot(), "menu.promptHint", "Type your answer in chat"));
}

}  // namespace AdminSystem::Menus
