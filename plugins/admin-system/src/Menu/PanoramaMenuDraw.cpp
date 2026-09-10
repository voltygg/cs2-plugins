#include "PanoramaMenu.hpp"

#include <VoltMod/Core/Slot.hpp>
#include <algorithm>
#include <array>
#include <format>
#include <string>
#include <string_view>

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

/** The accent a row kind carries, as an index into `AdminUi::Menu::AccentClasses`. */
static int AccentFor(const MenuRow& row)
{
    if (!row.Enabled)
        return -1;
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
        return -1;
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

    DrawHeader(slot, *menu);
    DrawTabs(slot);
    DrawRows(slot, *menu);
    DrawPrompt(slot);

    // The two footer buttons say what they do: a submenu goes back, a root menu closes.
    const bool deep = _stack.Depth(slot) > 1;
    Chrome.Back.Write(panel, slot, Translate(slot, deep ? "nav.back" : "nav.root", deep ? "Back" : "Main"));
    Chrome.Close.Write(panel, slot, Translate(slot, "nav.close", "Close"));
    Chrome.Cancel.Write(panel, slot, Translate(slot, "menu.cancel", "Cancel"));
}

void PanoramaMenu::DrawHeader(int slot, const Menu& menu)
{
    UiPanel& panel = _screen.Panel(slot);
    Chrome.Title.Write(panel, slot, menu.Title);
    Chrome.Crumb.Write(panel, slot, _stack.Breadcrumb(slot));
    Chrome.Subtitle.Write(panel, slot, menu.Subtitle);
    Chrome.SubtitleHidden.Write(panel, slot, menu.Subtitle.empty());
}

void PanoramaMenu::DrawTabs(int slot)
{
    UiPanel& panel = _screen.Panel(slot);
    const Session& session = _sessions[slot];

    for (int tab = 0; tab < TabCount; ++tab)
    {
        const bool used = tab < static_cast<int>(session.Tabs.size());
        Tabs[tab].Hidden.Write(panel, slot, !used);
        if (!used)
            continue;

        Tabs[tab].Label.Write(panel, slot, session.Tabs[tab].Label);
        Tabs[tab].Selected.Write(panel, slot, tab == session.OpenTab);
    }
}

void PanoramaMenu::DrawRows(int slot, const Menu& menu)
{
    UiPanel& panel = _screen.Panel(slot);
    Session& session = _sessions[slot];

    const int count = static_cast<int>(menu.Items.size());
    const int pages = VoltMod::PageCount(count, RowsPerPage);
    session.Page = std::clamp(session.Page, 0, pages - 1);

    for (int row = 0; row < RowsPerPage; ++row)
    {
        const int item = ItemIndex(slot, row);
        if (item >= count)
        {
            Rows[row].Hidden.Write(panel, slot, true);
            continue;
        }
        DrawRow(slot, row, _stack.Describe(slot, item));
    }

    Chrome.EmptyHidden.Write(panel, slot, count != 0);
    if (count == 0)
        Chrome.Empty.Write(panel, slot, Translate(slot, "menu.empty", "Nothing here"));

    Chrome.PagerHidden.Write(panel, slot, pages <= 1);
    if (pages > 1)
        Chrome.Page.Write(panel, slot, std::format("{} / {}", session.Page + 1, pages));
}

void PanoramaMenu::DrawRow(int slot, int row, const MenuRow& described)
{
    UiPanel& panel = _screen.Panel(slot);
    const RowIds& ids = Rows[row];

    ids.Hidden.Write(panel, slot, false);
    ids.Label.Write(panel, slot, described.Label);
    ids.Value.Write(panel, slot, described.Value);
    ids.HasValue.Write(panel, slot, !described.Value.empty());
    ids.Disabled.Write(panel, slot, !described.Enabled);
    ids.On.Write(panel, slot, described.State.value_or(false));
    ids.Kind.Write(panel, slot, static_cast<int>(described.Kind));
    ids.Accent.Write(panel, slot, AccentFor(described));

    // Steppers only where A/D would have done something, and only while the row is live.
    ids.HasSteppers.Write(panel, slot, described.Steppable && described.Enabled);

    // The hint line doubles as the "not applied yet" note, which is also what Changed pulses on.
    ids.HasHint.Write(panel, slot, described.Pending);
    ids.Changed.Write(panel, slot, described.Pending);
    if (described.Pending)
        ids.Hint.Write(panel, slot, Translate(slot, "menu.pending", "Applying..."));
}

void PanoramaMenu::DrawPrompt(int slot)
{
    UiPanel& panel = _screen.Panel(slot);
    const auto prompt = _rt.Hooks.ChatInput.GetPrompt(slot);

    Chrome.PromptHidden.Write(panel, slot, !prompt.has_value());
    Chrome.Prompting.Write(panel, slot, prompt.has_value());
    if (!prompt)
        return;

    Chrome.PromptText.Write(panel, slot, *prompt);
    Chrome.PromptHint.Write(panel, slot, Translate(slot, "menu.promptHint", "Type your answer in chat"));
}

}  // namespace AdminSystem::Menus
