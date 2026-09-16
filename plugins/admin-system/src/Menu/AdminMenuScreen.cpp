#include "Menu/AdminMenuScreen.hpp"

#include <cstddef>
#include <string>

using VoltMod::MenuButton;
using VoltMod::MenuButtonKind;
using VoltMod::MenuHeader;
using VoltMod::MenuRow;
using VoltMod::MenuRowKind;
using VoltMod::MenuTab;
using VoltMod::Screen;

namespace AdminSystem::Menus
{

AdminMenuScreen::AdminMenuScreen(VoltMod::ScreenManager& screens)
    : _screens(screens, std::string(AdminMenuLayout::Layout))
{}

bool AdminMenuScreen::Show(int slot)
{
    Screen& screen = _screens.For(slot);
    if (!screen.EnsureSpawned(slot))
        return false;

    screen.SetHidden(slot, AdminMenuLayout::RootId, false);
    screen.ShowCursor(slot, true);
    return true;
}

void AdminMenuScreen::Hide(int slot)
{
    Screen* screen = _screens.Find(slot);
    if (!screen || !*screen)
        return;

    screen->ShowCursor(slot, false);
    screen->SetHidden(slot, AdminMenuLayout::RootId, true);
}

void AdminMenuScreen::SetHeader(int slot, const MenuHeader& header)
{
    Screen& screen = _screens.For(slot);
    screen.SetText(slot, AdminMenuLayout::BrandVar, header.Brand);
    screen.SetText(slot, AdminMenuLayout::BrandSubtitleVar, header.BrandSubtitle);
    screen.SetText(slot, AdminMenuLayout::BreadcrumbVar, header.Breadcrumb);
    screen.SetText(slot, AdminMenuLayout::TitleVar, header.Title);
    screen.SetText(slot, AdminMenuLayout::SubtitleVar, header.Subtitle);
    screen.SetHidden(slot, AdminMenuLayout::Subtitle, header.Subtitle.empty());
}

void AdminMenuScreen::SetSidebarVisible(int slot, bool visible)
{
    _screens.For(slot).SetClass(slot, AdminMenuLayout::RootId, "NoSidebar", !visible);
}

void AdminMenuScreen::SetTab(int slot, int index, const MenuTab* tab)
{
    const AdminMenuLayout::Tab& ids = AdminMenuLayout::Tabs[static_cast<std::size_t>(index)];
    Screen& screen = _screens.For(slot);

    screen.SetHidden(slot, ids.Id, !tab);
    if (!tab)
        return;

    screen.SetText(slot, ids.Var, tab->Label);
    screen.SetClass(slot, ids.Id, "Selected", tab->Selected);

    // Every icon class is written, so the one a previous tab showed turns off.
    for (std::size_t icon = 0; icon < AdminMenuLayout::IconClasses.size(); ++icon)
        screen.SetClass(slot, ids.Icon, AdminMenuLayout::IconClasses[icon],
                        AdminMenuLayout::IconNames[icon] == tab->Icon);
}

void AdminMenuScreen::SetRow(int slot, int index, const MenuRow* row, std::string_view pendingHint)
{
    const AdminMenuLayout::Row& ids = AdminMenuLayout::Rows[static_cast<std::size_t>(index)];
    Screen& screen = _screens.For(slot);

    screen.SetHidden(slot, ids.Id, !row);
    if (!row)
        return;

    const bool toggle = row->Kind == MenuRowKind::Toggle;
    screen.SetText(slot, ids.LabelVar, row->Label);
    screen.SetText(slot, ids.ValueVar, row->Value);
    screen.SetClass(slot, ids.Id, "HasValue", !row->Value.empty());
    screen.SetClass(slot, ids.Id, "Disabled", !row->Enabled);
    screen.SetClass(slot, ids.Id, "On", row->State.value_or(false));
    screen.SetClass(slot, ids.Id, "Toggle", toggle);
    screen.SetClass(slot, ids.Id, "HasChevron", row->Kind == MenuRowKind::Submenu || row->Kind == MenuRowKind::Input);
    // A heading still ships a live button, so without this it hovers and clicks like any other row.
    screen.SetClass(slot, ids.Id, "Static", row->Kind == MenuRowKind::Text);
    screen.SetClass(slot, ids.Id, "HasSteppers", row->Steppable && row->Enabled && !toggle);
    screen.SetClass(slot, ids.Id, "Pending", row->Pending);
    if (row->Pending)
        screen.SetText(slot, ids.HintVar, pendingHint);
}

void AdminMenuScreen::SetEmpty(int slot, std::string_view text)
{
    Screen& screen = _screens.For(slot);
    screen.SetText(slot, AdminMenuLayout::EmptyVar, text);
    screen.SetHidden(slot, AdminMenuLayout::Empty, text.empty());
}

void AdminMenuScreen::SetPager(int slot, std::string_view text)
{
    Screen& screen = _screens.For(slot);
    screen.SetText(slot, AdminMenuLayout::PageVar, text);
    screen.SetHidden(slot, AdminMenuLayout::Page, text.empty());
}

void AdminMenuScreen::SetPrompt(int slot, std::string_view text, std::string_view hint)
{
    Screen& screen = _screens.For(slot);
    screen.SetText(slot, AdminMenuLayout::PromptTextVar, text);
    screen.SetText(slot, AdminMenuLayout::PromptHintVar, hint);
    screen.SetHidden(slot, AdminMenuLayout::Prompt, text.empty());
    screen.SetClass(slot, AdminMenuLayout::RootId, "Prompting", !text.empty());
}

void AdminMenuScreen::SetFooter(int slot, std::string_view back, std::string_view cancel)
{
    Screen& screen = _screens.For(slot);
    screen.SetText(slot, AdminMenuLayout::BackVar, back);
    screen.SetText(slot, AdminMenuLayout::CancelVar, cancel);
}

std::optional<MenuButton> AdminMenuScreen::ButtonFor(std::string_view id) const
{
    if (id == AdminMenuLayout::Cancel)
        return MenuButton{MenuButtonKind::Cancel};
    if (id == AdminMenuLayout::Back)
        return MenuButton{MenuButtonKind::Back};
    if (id == AdminMenuLayout::Close)
        return MenuButton{MenuButtonKind::Close};
    if (id == AdminMenuLayout::PagePrevious)
        return MenuButton{MenuButtonKind::PreviousPage};
    if (id == AdminMenuLayout::PageNext)
        return MenuButton{MenuButtonKind::NextPage};

    for (int index = 0; index < TabCount(); ++index)
    {
        if (id == AdminMenuLayout::Tabs[static_cast<std::size_t>(index)].Id)
            return MenuButton{MenuButtonKind::Tab, index};
    }

    for (int index = 0; index < RowCount(); ++index)
    {
        const AdminMenuLayout::Row& row = AdminMenuLayout::Rows[static_cast<std::size_t>(index)];
        if (id == row.Button)
            return MenuButton{MenuButtonKind::Row, index};
        if (id == row.Decrease)
            return MenuButton{MenuButtonKind::StepDown, index};
        if (id == row.Increase)
            return MenuButton{MenuButtonKind::StepUp, index};
    }

    return std::nullopt;
}

}  // namespace AdminSystem::Menus
