#include "Menu/AdminMenuScreen.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Slot.hpp>
#include <cstddef>
#include <utility>

using VoltMod::MenuRow;
using VoltMod::MenuRowKind;
using VoltMod::Screen;

namespace AdminSystem::Menus
{

AdminMenuScreen::AdminMenuScreen(VoltMod::ScreenManager& screens, VoltMod::SlotEvents& slots) : _manager(screens)
{
    _screens.BindReset(slots);
}

bool AdminMenuScreen::Show(int slot)
{
    Screen& screen = ScreenFor(slot);
    if (!screen.EnsureSpawned(slot))
        return false;

    screen.SetClass(slot, AdminMenuLayout::RootId, "Hidden", false);
    screen.ShowCursor(slot, true);
    return true;
}

void AdminMenuScreen::Hide(int slot)
{
    if (!VoltMod::IsValidSlot(slot) || !_screens[slot] || !*_screens[slot])
        return;

    Screen& screen = *_screens[slot];
    screen.ShowCursor(slot, false);
    screen.SetClass(slot, AdminMenuLayout::RootId, "Hidden", true);
}

void AdminMenuScreen::SetHeader(int slot, const MenuHeader& header)
{
    Screen& screen = ScreenFor(slot);
    screen.SetText(slot, AdminMenuLayout::BrandVar, header.Brand);
    screen.SetText(slot, AdminMenuLayout::BrandSubtitleVar, header.BrandSubtitle);
    screen.SetText(slot, AdminMenuLayout::BreadcrumbVar, header.Breadcrumb);
    screen.SetText(slot, AdminMenuLayout::TitleVar, header.Title);
    screen.SetText(slot, AdminMenuLayout::SubtitleVar, header.Subtitle);
    screen.SetClass(slot, AdminMenuLayout::Subtitle, "Hidden", header.Subtitle.empty());
}

void AdminMenuScreen::SetSidebarVisible(int slot, bool visible)
{
    ScreenFor(slot).SetClass(slot, AdminMenuLayout::RootId, "NoSidebar", !visible);
}

void AdminMenuScreen::SetTab(int slot, int index, std::string_view label, std::string_view icon, bool selected)
{
    const AdminMenuLayout::Tab& tab = AdminMenuLayout::Tabs[static_cast<std::size_t>(index)];
    Screen& screen = ScreenFor(slot);

    screen.SetClass(slot, tab.Id, "Hidden", false);
    screen.SetText(slot, tab.Var, label);
    screen.SetClass(slot, tab.Id, "Selected", selected);

    // Every icon class is written, so the one a previous tab showed turns off.
    for (std::string_view iconClass : AdminMenuLayout::IconClasses)
        screen.SetClass(slot, tab.Icon, iconClass, iconClass.substr(iconClass.find("--") + 2) == icon);
}

void AdminMenuScreen::HideTab(int slot, int index)
{
    ScreenFor(slot).SetClass(slot, AdminMenuLayout::Tabs[static_cast<std::size_t>(index)].Id, "Hidden", true);
}

void AdminMenuScreen::SetRow(int slot, int index, const MenuRow& row, std::string_view pendingHint)
{
    const AdminMenuLayout::Row& ids = AdminMenuLayout::Rows[static_cast<std::size_t>(index)];
    Screen& screen = ScreenFor(slot);
    const bool toggle = row.Kind == MenuRowKind::Toggle;

    screen.SetClass(slot, ids.Id, "Hidden", false);
    screen.SetText(slot, ids.LabelVar, row.Label);
    screen.SetText(slot, ids.ValueVar, row.Value);
    screen.SetClass(slot, ids.Id, "HasValue", !row.Value.empty());
    screen.SetClass(slot, ids.Id, "Disabled", !row.Enabled);
    screen.SetClass(slot, ids.Id, "On", row.State.value_or(false));
    screen.SetClass(slot, ids.Id, "Toggle", toggle);
    screen.SetClass(slot, ids.Id, "HasChevron", row.Kind == MenuRowKind::Submenu || row.Kind == MenuRowKind::Input);
    // A toggle flips on a press, so its switch is the whole control.
    screen.SetClass(slot, ids.Id, "HasSteppers", row.Steppable && row.Enabled && !toggle);
    screen.SetClass(slot, ids.Id, "Pending", row.Pending);
    if (row.Pending)
        screen.SetText(slot, ids.HintVar, pendingHint);
}

void AdminMenuScreen::HideRow(int slot, int index)
{
    ScreenFor(slot).SetClass(slot, AdminMenuLayout::Rows[static_cast<std::size_t>(index)].Id, "Hidden", true);
}

void AdminMenuScreen::ShowEmpty(int slot, std::string_view text)
{
    Screen& screen = ScreenFor(slot);
    screen.SetText(slot, AdminMenuLayout::EmptyVar, text);
    screen.SetClass(slot, AdminMenuLayout::Empty, "Hidden", false);
}

void AdminMenuScreen::HideEmpty(int slot)
{
    ScreenFor(slot).SetClass(slot, AdminMenuLayout::Empty, "Hidden", true);
}

void AdminMenuScreen::ShowPager(int slot, std::string_view text)
{
    Screen& screen = ScreenFor(slot);
    screen.SetText(slot, AdminMenuLayout::PageVar, text);
    screen.SetClass(slot, AdminMenuLayout::Page, "Hidden", false);
}

void AdminMenuScreen::HidePager(int slot)
{
    ScreenFor(slot).SetClass(slot, AdminMenuLayout::Page, "Hidden", true);
}

void AdminMenuScreen::ShowPrompt(int slot, std::string_view text, std::string_view hint)
{
    Screen& screen = ScreenFor(slot);
    screen.SetText(slot, AdminMenuLayout::PromptTextVar, text);
    screen.SetText(slot, AdminMenuLayout::PromptHintVar, hint);
    screen.SetClass(slot, AdminMenuLayout::Prompt, "Hidden", false);
    screen.SetClass(slot, AdminMenuLayout::RootId, "Prompting", true);
}

void AdminMenuScreen::HidePrompt(int slot)
{
    Screen& screen = ScreenFor(slot);
    screen.SetClass(slot, AdminMenuLayout::Prompt, "Hidden", true);
    screen.SetClass(slot, AdminMenuLayout::RootId, "Prompting", false);
}

void AdminMenuScreen::SetFooter(int slot, std::string_view back, std::string_view cancel)
{
    Screen& screen = ScreenFor(slot);
    screen.SetText(slot, AdminMenuLayout::BackVar, back);
    screen.SetText(slot, AdminMenuLayout::CancelVar, cancel);
}

std::optional<MenuButton> AdminMenuScreen::ButtonFor(std::string_view id)
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

    for (int index = 0; index < TabCount; ++index)
    {
        if (id == AdminMenuLayout::Tabs[static_cast<std::size_t>(index)].Id)
            return MenuButton{MenuButtonKind::Tab, index};
    }

    for (int index = 0; index < RowCount; ++index)
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

Screen& AdminMenuScreen::ScreenFor(int slot)
{
    if (!VoltMod::IsValidSlot(slot))
        return _empty;

    std::optional<Screen>& screen = _screens[slot];
    if (!screen)
    {
        auto created = _manager.ForPlayer(AdminMenuLayout::Layout, slot);
        if (!created)
        {
            VoltMod::Log::Warn("Admin menu: no screen for slot {} ({}).", slot, created.error().Detail);
            return _empty;
        }
        screen.emplace(std::move(*created));
    }
    return *screen;
}

}  // namespace AdminSystem::Menus
