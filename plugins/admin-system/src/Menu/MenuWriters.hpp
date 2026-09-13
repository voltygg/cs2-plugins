#pragma once

#include <Ui/AdminMenu.hpp>
#include <VoltMod/Ui/Writers.hpp>
#include <array>
#include <string_view>

namespace AdminSystem::Menus
{

/** Rows one page holds and tabs over the root menu: as many as the layout ships. */
inline constexpr int RowsPerPage = static_cast<int>(AdminUi::Menu::Rows.size());
inline constexpr int TabCount = static_cast<int>(AdminUi::Menu::Tabs.size());

/** Every writer one row needs. */
struct RowWriters
{
    VoltMod::TextVar Label;
    VoltMod::TextVar Hint;
    VoltMod::TextVar Value;
    VoltMod::ClassFlag Hidden;
    VoltMod::ClassFlag Disabled;
    VoltMod::ClassFlag HasValue;
    VoltMod::ClassFlag HasHint;
    VoltMod::ClassFlag HasSteppers;
    VoltMod::ClassFlag On;
    VoltMod::ClassFlag Changed;
    VoltMod::ClassFlag Toggle;
    VoltMod::ClassFlag HasChevron;
};

constexpr RowWriters MakeRow(const AdminUi::Menu::Row& row)
{
    namespace Screen = AdminUi::Menu;
    return {
        .Label = {Screen::RootId, row.LabelVar},
        .Hint = {Screen::RootId, row.HintVar},
        .Value = {Screen::RootId, row.ValueVar},
        .Hidden = {row.Id, "Hidden"},
        .Disabled = {row.Id, "Disabled"},
        .HasValue = {row.Id, "HasValue"},
        .HasHint = {row.Id, "HasHint"},
        .HasSteppers = {row.Id, "HasSteppers"},
        .On = {row.Id, "On"},
        .Changed = {row.Id, "Changed"},
        .Toggle = {row.Id, "Toggle"},
        .HasChevron = {row.Id, "HasChevron"},
    };
}

inline constexpr auto Rows = VoltMod::MakeWriters(AdminUi::Menu::Rows, MakeRow);

struct TabWriters
{
    VoltMod::TextVar Label;
    VoltMod::ClassChoice Icon;
    VoltMod::ClassFlag Hidden;
    VoltMod::ClassFlag Selected;
};

constexpr TabWriters MakeTab(const AdminUi::Menu::Tab& tab)
{
    return {
        .Label = {AdminUi::Menu::RootId, tab.Var},
        .Icon = {tab.Icon, AdminUi::Menu::IconClasses},
        .Hidden = {tab.Id, "Hidden"},
        .Selected = {tab.Id, "Selected"},
    };
}

inline constexpr auto Tabs = VoltMod::MakeWriters(AdminUi::Menu::Tabs, MakeTab);

/** Everything outside the row and tab pools. */
struct ShellWriters
{
    VoltMod::TextVar Brand{AdminUi::Menu::RootId, AdminUi::Menu::BrandVar};
    VoltMod::TextVar BrandSubtitle{AdminUi::Menu::RootId, AdminUi::Menu::BrandSubVar};
    VoltMod::TextVar Breadcrumb{AdminUi::Menu::RootId, AdminUi::Menu::CrumbVar};
    VoltMod::TextVar Title{AdminUi::Menu::RootId, AdminUi::Menu::TitleVar};
    VoltMod::TextVar Subtitle{AdminUi::Menu::RootId, AdminUi::Menu::SubtitleVar};
    VoltMod::TextVar Page{AdminUi::Menu::RootId, AdminUi::Menu::PageVar};
    VoltMod::TextVar Empty{AdminUi::Menu::RootId, AdminUi::Menu::EmptyVar};
    VoltMod::TextVar PromptText{AdminUi::Menu::RootId, AdminUi::Menu::PromptTextVar};
    VoltMod::TextVar PromptHint{AdminUi::Menu::RootId, AdminUi::Menu::PromptHintVar};
    VoltMod::TextVar Back{AdminUi::Menu::RootId, AdminUi::Menu::BackVar};
    VoltMod::TextVar Cancel{AdminUi::Menu::RootId, AdminUi::Menu::CancelVar};

    VoltMod::ClassFlag SubtitleHidden{AdminUi::Menu::Subtitle, "Hidden"};
    VoltMod::ClassFlag EmptyHidden{AdminUi::Menu::Empty, "Hidden"};
    VoltMod::ClassFlag PagerHidden{AdminUi::Menu::Page, "Hidden"};
    VoltMod::ClassFlag PromptHidden{AdminUi::Menu::Prompt, "Hidden"};
    VoltMod::ClassFlag Prompting{AdminUi::Menu::RootId, "Prompting"};
};

inline constexpr ShellWriters Shell{};

}  // namespace AdminSystem::Menus
