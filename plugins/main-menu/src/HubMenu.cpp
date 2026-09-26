#include "HubMenu.hpp"

#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Messaging/ChatColors.hpp>
#include <algorithm>
#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ChatColors = VoltMod::ChatColors;
using VoltMod::ChoiceRow;
using VoltMod::MenuBuilder;
using VoltMod::MenuItem;
using VoltMod::MenuRow;
using VoltMod::MenuRowKind;
using VoltMod::MenuSurface;
using VoltMod::SubmenuRow;

namespace MainMenu
{

std::shared_ptr<VoltMod::Menu> HubMenu::Build(int slot)
{
    MenuBuilder builder(Text(slot, "menu.title"));
    builder.Subtitle(Text(slot, "menu.subtitle"));

    for (const Tab& tab : _config.Get().tabs)
    {
        const bool visible =
            std::ranges::any_of(tab.entries, [&](const Entry& entry) { return IsVisible(entry, slot); });
        if (!visible)
        {
            continue;
        }

        // A copy, so a config reload while the menu is open cannot pull the tab out from under it.
        builder.Add(SubmenuRow{.Label = Text(slot, tab.label),
                               .Build = [this, tab](int opener) { return BuildTab(tab, opener); },
                               .Icon = tab.icon});
    }

    builder.Add(SubmenuRow{.Label = Text(slot, "tab.settings"),
                           .Build = [this](int opener) { return BuildSettings(opener); },
                           .Icon = "settings"});
    return builder.Build();
}

std::shared_ptr<VoltMod::Menu> HubMenu::BuildSettings(int slot)
{
    // "" is the server's language.
    std::vector<VoltMod::Labeled<std::string>> choices{{.Label = Text(slot, "language.default"), .Value = ""}};
    const std::string_view current = _translations.PlayerLanguage(slot);
    int index = 0;
    std::vector<std::string> codes = _translations.GetAvailableLanguages();
    std::ranges::sort(codes);
    for (const std::string& code : codes)
    {
        if (code == current)
        {
            index = static_cast<int>(choices.size());
        }
        choices.push_back({.Label = _translations.GetOr("language." + code, slot, code), .Value = code});
    }

    MenuBuilder builder(Text(slot, "tab.settings"));
    builder.Add(ChoiceRow<std::string>{
        .Label = Text(slot, "settings.language"),
        .Choices = std::move(choices),
        .Commit = [this](int player, const std::string& lang) { SetLanguage(player, lang); },
        .Index = index,
    });
    return builder.Build();
}

void HubMenu::SetLanguage(int slot, const std::string& lang)
{
    _translations.SetPlayerLanguage(slot, lang);
    Open(slot);
    _menus.Open(slot, BuildSettings(slot));
}

void HubMenu::Open(int slot)
{
    // Players reach it mid-round, where being held still is worse than stray movement.
    _menus.OpenSession(slot, Build(slot), {.FreezeMovement = false, .HomePage = true});
}

std::shared_ptr<VoltMod::Menu> HubMenu::BuildTab(const Tab& tab, int slot)
{
    MenuBuilder builder(Text(slot, tab.label));
    for (const Entry& entry : tab.entries)
    {
        if (IsVisible(entry, slot))
        {
            builder.Add(Row(entry));
        }
    }
    return builder.Build();
}

MenuItem HubMenu::Row(const Entry& entry)
{
    auto shared = std::make_shared<const Entry>(entry);
    return MenuItem{
        .Describe =
            [this, shared](int slot) {
                MenuRow row{.Label = Text(slot, shared->label)};
                if (shared->kind == EntryKind::Link)
                {
                    std::string_view url = shared->target;
                    row.Value = url.substr(url.find("://") + 3);
                }
                else
                {
                    // Draws the chevron: the row leaves this menu for another one.
                    row.Kind = MenuRowKind::Submenu;
                }
                return row;
            },
        .Activate = [this, shared](int slot, MenuSurface& surface) { Run(*shared, slot, surface); },
    };
}

void HubMenu::Run(const Entry& entry, int slot, MenuSurface& surface)
{
    if (entry.kind == EntryKind::Link)
    {
        _messages.SendKey(
            slot, "link.message",
            {{"label", Text(slot, entry.label)}, {"url", std::format("{}{}", ChatColors::LightBlue, entry.target)}});
        return;
    }

    if (entry.kind == EntryKind::Command)
    {
        surface.CloseAll(slot);
        _entities.Controller(slot).ExecuteCommand(entry.target);
        return;
    }

    OpenSection(entry.target, slot, surface);
}

void HubMenu::OpenSection(std::string_view id, int slot, MenuSurface& surface)
{
    Contracts::IMenuSection* section = Section(id);
    if (!section || !section->IsVisibleTo(slot))
    {
        _messages.SendKey(slot, "entry.unavailable");
        return;
    }
    surface.CloseAll(slot);
    if (!section->Open(slot))
    {
        _messages.SendKey(slot, "entry.unavailable");
    }
}

bool HubMenu::IsVisible(const Entry& entry, int slot)
{
    if (entry.kind != EntryKind::Section)
    {
        return true;
    }
    Contracts::IMenuSection* section = Section(entry.target);
    return section && section->IsVisibleTo(slot);
}

Contracts::IMenuSection* HubMenu::Section(std::string_view id)
{
    return _exchange.Get<Contracts::IMenuSection>(id);
}

std::string HubMenu::Text(int slot, std::string_view label) const
{
    return _translations.GetOr(label, slot, label);
}

}  // namespace MainMenu
