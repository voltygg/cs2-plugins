#include "HubMenu.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <algorithm>
#include <utility>

namespace Log = VoltMod::Log;
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

    for (const TabSettings& tab : _config.Get().tabs)
    {
        const bool visible =
            std::ranges::any_of(tab.entries, [&](const EntrySettings& entry) { return IsVisible(entry, slot); });
        if (!visible)
            continue;

        // A copy, so a config reload while the menu is open cannot pull the tab out from under it.
        builder.Add(SubmenuRow{.Label = Text(slot, tab.label),
                               .Build = [this, tab](int opener) { return BuildTab(tab, opener); },
                               .Icon = tab.icon});
    }
    return builder.Build();
}

std::shared_ptr<VoltMod::Menu> HubMenu::BuildTab(const TabSettings& tab, int slot)
{
    MenuBuilder builder(Text(slot, tab.label));
    for (const EntrySettings& entry : tab.entries)
    {
        if (IsVisible(entry, slot))
            builder.Add(Row(entry));
    }
    return builder.Build();
}

MenuItem HubMenu::Row(const EntrySettings& entry)
{
    return MenuItem{
        .Describe =
            [this, entry](int slot) {
                MenuRow row{.Label = Text(slot, entry.label)};
                if (entry.kind == "link")
                {
                    std::string_view url = entry.url;
                    row.Value = url.substr(url.find("://") + 3);
                }
                else
                {
                    // Draws the chevron: the row leaves this menu for another one.
                    row.Kind = MenuRowKind::Submenu;
                }
                return row;
            },
        .Activate = [this, entry](int slot, MenuSurface& surface) { Run(entry, slot, surface); },
    };
}

void HubMenu::Run(const EntrySettings& entry, int slot, MenuSurface& surface)
{
    if (entry.kind == "link")
    {
        _messages.ReplyKey(slot, "link.message", {{"label", Text(slot, entry.label)}, {"url", entry.url}});
        return;
    }

    if (entry.kind == "command")
    {
        surface.CloseAll(slot);
        if (auto ran = _conVars.ExecuteClientCommand(slot, entry.command); !ran)
        {
            Log::Warn("'{}' did not run: {}", entry.command, ran.error().Detail);
            _messages.ReplyKey(slot, "entry.unavailable");
        }
        return;
    }

    Contracts::IMenuSection* section = Section(entry.section);
    if (!section || !section->IsVisibleTo(slot))
    {
        _messages.ReplyKey(slot, "entry.unavailable");
        return;
    }
    surface.CloseAll(slot);
    if (!section->Open(slot))
        _messages.ReplyKey(slot, "entry.unavailable");
}

bool HubMenu::IsVisible(const EntrySettings& entry, int slot)
{
    if (entry.kind != "section")
        return true;
    Contracts::IMenuSection* section = Section(entry.section);
    return section && section->IsVisibleTo(slot);
}

Contracts::IMenuSection* HubMenu::Section(std::string_view id)
{
    return static_cast<Contracts::IMenuSection*>(_exchange.Find(Contracts::MenuSectionName(id)));
}

std::string HubMenu::Text(int slot, std::string_view label) const
{
    return _translations.GetOr(label, slot, label);
}

}  // namespace MainMenu
