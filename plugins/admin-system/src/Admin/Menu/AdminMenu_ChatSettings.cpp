#include "AdminMenu_ChatSettings.hpp"

#include "../../Core/App.hpp"
#include "../AdminManager.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/ChatColors.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Menu/MenuManager.hpp>
#include <VoltMod/Menu/MenuPresets.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_map>

namespace AdminSystem::Admin::Menu
{

using VoltMod::Core::Translations;
using VoltMod::Menu::ChoiceOption;
using VoltMod::Menu::MenuBuilder;
using VoltMod::Menu::MenuManager;
using VoltMod::Players::PlayerManager;

namespace
{

namespace ChatColors = VoltMod::Core::ChatColors;

// Translation-key suffix per canonical VoltMod color name. Compound names ("lightblue")
// don't survive a generic "uppercase first letter" rule, so the small lookup keeps the
// JSON keys stable and lets us reuse the framework's @ref ChatColors::Palette for everything else.
const std::unordered_map<std::string_view, std::string_view>& ColorLabelKeys()
{
    static const std::unordered_map<std::string_view, std::string_view> kKeys = {
        {"default", "color.default"}, {"darkred", "color.darkRed"},     {"lightpurple", "color.lightPurple"},
        {"green", "color.green"},     {"olive", "color.olive"},         {"lime", "color.lime"},
        {"red", "color.red"},         {"gray", "color.gray"},           {"yellow", "color.yellow"},
        {"silver", "color.silver"},   {"lightblue", "color.lightBlue"}, {"darkblue", "color.darkBlue"},
        {"purple", "color.purple"},   {"lightred", "color.lightRed"},   {"gold", "color.gold"},
    };
    return kKeys;
}

// The DB stores empty string when the admin wants their group default; index 0 in the
// rendered choice list maps to that empty value. We then append every VoltMod canonical
// color, so the list automatically grows when new colors are added upstream.
int IndexForColor(std::string_view color)
{
    if (color.empty())
        return 0;
    for (size_t i = 0; i < ChatColors::Palette.size(); ++i)
    {
        if (ChatColors::Palette[i].Name == color)
            return static_cast<int>(i + 1);
    }

    return 0;
}

std::vector<ChoiceOption<std::string>::Choice> BuildColorChoices(App& app, int viewerSlot)
{
    auto& tr = app.Runtime.Translations;
    const auto& keys = ColorLabelKeys();

    std::vector<ChoiceOption<std::string>::Choice> choices;
    choices.reserve(ChatColors::Palette.size() + 1);

    // First entry is always "(group default)" - empty DB value, distinct from
    // ChatColors::Default which is itself a real overrideable color.
    choices.push_back({tr.Get("color.groupDefault", viewerSlot), std::string{}});

    // The framework renders the palette; colors without a translation key fall back to their name.
    auto palette = ::VoltMod::Menu::BuildPaletteChoices([&](std::string_view name) -> std::string {
        if (auto it = keys.find(name); it != keys.end())
            return tr.Get(std::string(it->second), viewerSlot);
        return {};
    });
    choices.insert(choices.end(), std::make_move_iterator(palette.begin()), std::make_move_iterator(palette.end()));
    return choices;
}

enum class ColorSlot
{
    Name,
    Message,
};

// Pull the current persisted color out of the admin row (NOT the resolved style - we
// want the override slot itself, so "default" stays selected after a rebroadcast).
std::string CurrentSlotColor(App& app, int64_t steamId, ColorSlot slot)
{
    const auto* admin = app.Admins.GetAdmin(steamId);
    if (!admin)
        return "";
    switch (slot)
    {
    case ColorSlot::Name:
        return admin->NameColor;
    case ColorSlot::Message:
        return admin->MessageColor;
    }
    return "";
}

void AddColorChoice(App& app, MenuBuilder& builder, const std::string& title, int64_t steamId, ColorSlot slot,
                    int viewerSlot)
{
    auto choices = BuildColorChoices(app, viewerSlot);
    int initialIndex = IndexForColor(CurrentSlotColor(app, steamId, slot));

    builder.AddChoice<std::string>(
        title, std::move(choices),
        [&app, steamId, slot](int /*menuSlot*/, const std::string& value) {
            auto& mgr = app.Admins;
            const auto* admin = mgr.GetAdmin(steamId);
            if (!admin)
                return;
            std::string nameColor = admin->NameColor;
            std::string messageColor = admin->MessageColor;
            switch (slot)
            {
            case ColorSlot::Name:
                nameColor = value;
                break;
            case ColorSlot::Message:
                messageColor = value;
                break;
            }
            mgr.UpdateChatStyle(steamId, admin->DisplayPrefix, nameColor, messageColor);
        },
        true, initialIndex);
}

// Friendly name for a language code, keyed as `lang.<code>` so a new translations/<code>.json
// only needs a matching `lang.<code>` entry - no code change. Falls back to the raw code.
std::string LanguageLabel(App& app, const std::string& code, int viewerSlot)
{
    std::string key = "lang." + code;
    std::string label = app.Runtime.Translations.Get(key, viewerSlot);
    return label == key ? code : label;
}

// Sorted so the choice order is stable across the rebuild we trigger on commit.
std::vector<std::string> AvailableLanguagesSorted(App& app)
{
    auto langs = app.Runtime.Translations.GetAvailableLanguages();
    std::sort(langs.begin(), langs.end());
    return langs;
}

// Find the index of `code` in `langs`, or 0 if it's not found. Index 0 maps to the empty string
int IndexForLanguage(const std::vector<std::string>& langs, const std::string& code)
{
    auto it = std::find(langs.begin(), langs.end(), code);
    return it != langs.end() ? static_cast<int>(it - langs.begin()) : 0;
}

void AddLanguageChoice(App& app, MenuBuilder& builder, int64_t steamId, int viewerSlot)
{
    auto langs = AvailableLanguagesSorted(app);

    std::vector<ChoiceOption<std::string>::Choice> choices;
    choices.reserve(langs.size());

    for (const auto& code : langs)
    {
        choices.push_back({LanguageLabel(app, code, viewerSlot), code});
    }

    const auto* admin = app.Admins.GetAdmin(steamId);
    int initialIndex = IndexForLanguage(langs, admin ? admin->Language : std::string("en"));

    builder.AddChoice<std::string>(
        app.Runtime.Translations.Get("chat.panelLanguage", viewerSlot), std::move(choices),
        [&app, steamId](int menuSlot, const std::string& lang) {
            app.Admins.UpdateLanguage(steamId, lang);
            app.Runtime.Translations.SetPlayerLanguage(menuSlot, lang);
            // Rebuild so the baked labels re-render in the new language. Use the by-value
            // menuSlot (not a capture): CloseMenu frees this option and its captures, so
            // nothing read after it may live in the lambda's closure.
            auto& mgr = app.Runtime.Menus;
            mgr.CloseMenu(menuSlot);
            mgr.OpenMenu(menuSlot, BuildChatSettingsMenu(app, menuSlot));
        },
        true, initialIndex);
}

}  // namespace

std::shared_ptr<VoltMod::MenuView> BuildChatSettingsMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;
    auto* admin = app.Runtime.Players.GetPlayerBySlot(adminSlot);
    if (!admin)
        return nullptr;
    int64_t steamId = admin->GetSteamID();

    MenuBuilder builder(tr.Get("category.chatSettings", adminSlot));

    // Display Prefix toggle - flips the bool and persists immediately so the next chat line
    // reflects the change without needing an explicit "Save" row.
    builder.AddToggle(
        tr.Get("chat.displayPrefix", adminSlot), tr.Get("effectState.on", adminSlot),
        tr.Get("effectState.off", adminSlot),
        [&app, steamId](int) {
            const auto* a = app.Admins.GetAdmin(steamId);
            return a ? a->DisplayPrefix : true;
        },
        [&app, steamId](int) {
            auto& mgr = app.Admins;
            const auto* a = mgr.GetAdmin(steamId);
            if (!a)
                return;
            mgr.UpdateChatStyle(steamId, !a->DisplayPrefix, a->NameColor, a->MessageColor);
        });

    // Each choice row owns its cycle index (seeded from the persisted value); commit on E
    // persists that single slot.
    AddColorChoice(app, builder, tr.Get("chat.nameColor", adminSlot), steamId, ColorSlot::Name, adminSlot);
    AddColorChoice(app, builder, tr.Get("chat.messageColor", adminSlot), steamId, ColorSlot::Message, adminSlot);

    // Panel language - commit on E persists and rebuilds the menu in the chosen language.
    AddLanguageChoice(app, builder, steamId, adminSlot);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
