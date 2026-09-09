#pragma once

#include <Ui/AdminMenu.hpp>
#include <VoltMod/Ui/Widgets.hpp>
#include <array>
#include <string_view>

namespace AdminSystem::Menus
{

/** Rows one page of the screen holds. The layout ships this many row panels. */
inline constexpr int RowsPerPage = 8;

/** Tabs over the root menu's submenus. The layout ships this many tab buttons. */
inline constexpr int TabCount = 6;

/** Every writer one row needs, plus the ids its three buttons report a press under. */
struct RowIds
{
    VoltMod::Text Label;
    VoltMod::Text Hint;
    VoltMod::Text Value;
    VoltMod::Flag Hidden;
    VoltMod::Flag Disabled;
    VoltMod::Flag HasValue;
    VoltMod::Flag HasHint;
    VoltMod::Flag HasSteppers;
    VoltMod::Flag On;
    VoltMod::Flag Changed;
    VoltMod::Choice Kind;
    VoltMod::Choice Accent;

    std::string_view Press;
    std::string_view Dec;
    std::string_view Inc;
};

/** One row's writers, from the ids and variables `panorama render` emitted for it. */
constexpr RowIds MakeRow(std::string_view panel, std::string_view accent, std::string_view press,
                         std::string_view dec, std::string_view inc, std::string_view label,
                         std::string_view hint, std::string_view value)
{
    namespace Screen = AdminUi::Menu;
    return {
        .Label = {Screen::RootId, label},
        .Hint = {Screen::RootId, hint},
        .Value = {Screen::RootId, value},
        .Hidden = {panel, "Hidden"},
        .Disabled = {panel, "Disabled"},
        .HasValue = {panel, "HasValue"},
        .HasHint = {panel, "HasHint"},
        .HasSteppers = {panel, "HasSteppers"},
        .On = {panel, "On"},
        .Changed = {panel, "Changed"},
        .Kind = {panel, Screen::KindClasses},
        .Accent = {accent, Screen::AccentClasses},
        .Press = press,
        .Dec = dec,
        .Inc = inc,
    };
}

inline constexpr std::array<RowIds, RowsPerPage> Rows{
    MakeRow(AdminUi::Menu::Row0, AdminUi::Menu::Row0Accent, AdminUi::Menu::Row0Btn, AdminUi::Menu::Row0Dec,
            AdminUi::Menu::Row0Inc, AdminUi::Menu::Row0LabelVar, AdminUi::Menu::Row0HintVar,
            AdminUi::Menu::Row0ValueVar),
    MakeRow(AdminUi::Menu::Row1, AdminUi::Menu::Row1Accent, AdminUi::Menu::Row1Btn, AdminUi::Menu::Row1Dec,
            AdminUi::Menu::Row1Inc, AdminUi::Menu::Row1LabelVar, AdminUi::Menu::Row1HintVar,
            AdminUi::Menu::Row1ValueVar),
    MakeRow(AdminUi::Menu::Row2, AdminUi::Menu::Row2Accent, AdminUi::Menu::Row2Btn, AdminUi::Menu::Row2Dec,
            AdminUi::Menu::Row2Inc, AdminUi::Menu::Row2LabelVar, AdminUi::Menu::Row2HintVar,
            AdminUi::Menu::Row2ValueVar),
    MakeRow(AdminUi::Menu::Row3, AdminUi::Menu::Row3Accent, AdminUi::Menu::Row3Btn, AdminUi::Menu::Row3Dec,
            AdminUi::Menu::Row3Inc, AdminUi::Menu::Row3LabelVar, AdminUi::Menu::Row3HintVar,
            AdminUi::Menu::Row3ValueVar),
    MakeRow(AdminUi::Menu::Row4, AdminUi::Menu::Row4Accent, AdminUi::Menu::Row4Btn, AdminUi::Menu::Row4Dec,
            AdminUi::Menu::Row4Inc, AdminUi::Menu::Row4LabelVar, AdminUi::Menu::Row4HintVar,
            AdminUi::Menu::Row4ValueVar),
    MakeRow(AdminUi::Menu::Row5, AdminUi::Menu::Row5Accent, AdminUi::Menu::Row5Btn, AdminUi::Menu::Row5Dec,
            AdminUi::Menu::Row5Inc, AdminUi::Menu::Row5LabelVar, AdminUi::Menu::Row5HintVar,
            AdminUi::Menu::Row5ValueVar),
    MakeRow(AdminUi::Menu::Row6, AdminUi::Menu::Row6Accent, AdminUi::Menu::Row6Btn, AdminUi::Menu::Row6Dec,
            AdminUi::Menu::Row6Inc, AdminUi::Menu::Row6LabelVar, AdminUi::Menu::Row6HintVar,
            AdminUi::Menu::Row6ValueVar),
    MakeRow(AdminUi::Menu::Row7, AdminUi::Menu::Row7Accent, AdminUi::Menu::Row7Btn, AdminUi::Menu::Row7Dec,
            AdminUi::Menu::Row7Inc, AdminUi::Menu::Row7LabelVar, AdminUi::Menu::Row7HintVar,
            AdminUi::Menu::Row7ValueVar),
};

/** One tab: the button it reports a press under, its label, and its selected class. */
struct TabIds
{
    VoltMod::Text Label;
    VoltMod::Flag Hidden;
    VoltMod::Flag Selected;
    std::string_view Press;
};

constexpr TabIds MakeTab(std::string_view panel, std::string_view label)
{
    return {
        .Label = {AdminUi::Menu::RootId, label},
        .Hidden = {panel, "Hidden"},
        .Selected = {panel, "Selected"},
        .Press = panel,
    };
}

inline constexpr std::array<TabIds, TabCount> Tabs{
    MakeTab(AdminUi::Menu::Tab0, AdminUi::Menu::Tab0Var), MakeTab(AdminUi::Menu::Tab1, AdminUi::Menu::Tab1Var),
    MakeTab(AdminUi::Menu::Tab2, AdminUi::Menu::Tab2Var), MakeTab(AdminUi::Menu::Tab3, AdminUi::Menu::Tab3Var),
    MakeTab(AdminUi::Menu::Tab4, AdminUi::Menu::Tab4Var), MakeTab(AdminUi::Menu::Tab5, AdminUi::Menu::Tab5Var),
};

/** Everything outside the row and tab pools: the header, the pager, the prompt, the footer. */
struct ChromeIds
{
    VoltMod::Text Crumb{AdminUi::Menu::RootId, AdminUi::Menu::PanelCrumbVar};
    VoltMod::Text Title{AdminUi::Menu::RootId, AdminUi::Menu::PanelTitleVar};
    VoltMod::Text Subtitle{AdminUi::Menu::RootId, AdminUi::Menu::PanelSubtitleVar};
    VoltMod::Text Page{AdminUi::Menu::RootId, AdminUi::Menu::PageVar};
    VoltMod::Text Empty{AdminUi::Menu::RootId, AdminUi::Menu::EmptyVar};
    VoltMod::Text PromptText{AdminUi::Menu::RootId, AdminUi::Menu::PromptTextVar};
    VoltMod::Text PromptHint{AdminUi::Menu::RootId, AdminUi::Menu::PromptHintVar};
    VoltMod::Text Back{AdminUi::Menu::RootId, AdminUi::Menu::BackVar};
    VoltMod::Text Close{AdminUi::Menu::RootId, AdminUi::Menu::CloseVar};
    VoltMod::Text Cancel{AdminUi::Menu::RootId, AdminUi::Menu::CancelVar};

    VoltMod::Flag SubtitleHidden{AdminUi::Menu::PanelSubtitle, "Hidden"};
    VoltMod::Flag EmptyHidden{AdminUi::Menu::Empty, "Hidden"};
    VoltMod::Flag PagerHidden{AdminUi::Menu::Page, "Hidden"};
    VoltMod::Flag PromptHidden{AdminUi::Menu::Prompt, "Hidden"};
    VoltMod::Flag Prompting{AdminUi::Menu::RootId, "Prompting"};
};

inline constexpr ChromeIds Chrome{};

}  // namespace AdminSystem::Menus
