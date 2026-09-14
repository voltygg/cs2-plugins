#include "Menu/PanoramaMenu.hpp"

#include <VoltMod/Core/Slot.hpp>
#include <algorithm>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>

using VoltMod::Menu;
using VoltMod::MenuRow;
using VoltMod::MenuRowKind;
using VoltMod::UiPanel;

namespace AdminSystem::Menus
{

void PanoramaMenu::Draw(int slot)
{
    const Menu* menu = _stack.Current(slot);
    UiPanel& panel = _screen.Panel(slot);
    if (!menu || !panel)
        return;

    const VoltMod::UiPanelWriter w{panel, slot};
    DrawHeader(w, *menu);
    DrawTabs(w);
    DrawRows(w, *menu);
    DrawPrompt(w);

    const bool deep = _stack.Depth(slot) > 1;
    w.Set(Shell.Back, Translate(slot, deep ? "nav.back" : "nav.root", deep ? "Back" : "Main"));
    w.Set(Shell.Cancel, Translate(slot, "menu.cancel", "Cancel"));
}

void PanoramaMenu::DrawHeader(const VoltMod::UiPanelWriter& w, const Menu& menu)
{
    const Menu* root = _stack.Root(w.Slot());
    const Menu& brand = root ? *root : menu;
    w.Set(Shell.Brand, brand.Title);
    w.Set(Shell.BrandSubtitle, brand.Subtitle);

    w.Set(Shell.Title, menu.Title);
    w.Set(Shell.Breadcrumb, _stack.Breadcrumb(w.Slot()));
    // The root's subtitle is already under the brand.
    const bool subtitle = !menu.Subtitle.empty() && &menu != &brand;
    w.Set(Shell.Subtitle, menu.Subtitle);
    w.Set(Shell.SubtitleHidden, !subtitle);
}

void PanoramaMenu::DrawTabs(const VoltMod::UiPanelWriter& w)
{
    const Session& session = _sessions[w.Slot()];

    for (int tab = 0; tab < TabCount; ++tab)
    {
        const TabWriters& ids = Tabs[static_cast<std::size_t>(tab)];
        const bool used = tab < static_cast<int>(session.Tabs.size());
        w.Set(ids.Hidden, !used);
        if (!used)
            continue;

        const Tab& shown = session.Tabs[static_cast<std::size_t>(tab)];
        w.Set(ids.Label, shown.Label);
        w.Set(ids.Icon, shown.Icon);
        w.Set(ids.Selected, tab == session.SelectedTab);
    }
}

void PanoramaMenu::DrawRows(const VoltMod::UiPanelWriter& w, const Menu& menu)
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

void PanoramaMenu::DrawRow(const VoltMod::UiPanelWriter& w, int row, const MenuRow& described)
{
    const RowWriters& ids = Rows[static_cast<std::size_t>(row)];
    const bool toggle = described.Kind == MenuRowKind::Toggle;

    w.Set(ids.Hidden, false);
    w.Set(ids.Label, described.Label);
    w.Set(ids.Value, described.Value);
    w.Set(ids.HasValue, !described.Value.empty());
    w.Set(ids.Disabled, !described.Enabled);
    w.Set(ids.On, described.State.value_or(false));
    w.Set(ids.Toggle, toggle);
    w.Set(ids.HasChevron, described.Kind == MenuRowKind::Submenu || described.Kind == MenuRowKind::Input);

    // A toggle flips on a click, so its switch is the whole control.
    w.Set(ids.HasSteppers, described.Steppable && described.Enabled && !toggle);

    // The hint line doubles as the "not applied yet" note.
    w.Set(ids.HasHint, described.Pending);
    w.Set(ids.Changed, described.Pending);
    if (described.Pending)
        w.Set(ids.Hint, Translate(w.Slot(), "menu.pending", "Applying..."));
}

void PanoramaMenu::DrawPrompt(const VoltMod::UiPanelWriter& w)
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
