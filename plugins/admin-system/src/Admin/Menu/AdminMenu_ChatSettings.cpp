#include "AdminMenu_ChatSettings.hpp"

#include "../../Core/App.hpp"
#include "../AdminManager.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Menu/MenuManager.hpp>
#include <VoltMod/Menu/MenuPresets.hpp>
#include <VoltMod/Messaging/ChatColors.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_map>

namespace AdminSystem::Admin::Menu
{

using VoltMod::ChoiceOption;
using VoltMod::MenuBuilder;
using VoltMod::MenuManager;
using VoltMod::PlayerManager;
using VoltMod::Translations;

namespace ChatColors = VoltMod::ChatColors;

// Explicit keys keep compound color names stable across palette changes.
static const std::unordered_map<std::string_view, std::string_view>& ColorLabelKeys()
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

// Empty means inherit from the group and maps to choice index zero.
static int IndexForColor(std::string_view color)
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

static std::vector<ChoiceOption<std::string>::Choice> BuildColorChoices(App& app, int viewerSlot)
{
    auto& tr = app.Runtime.Translations;
    const auto& keys = ColorLabelKeys();

    std::vector<ChoiceOption<std::string>::Choice> choices;
    choices.reserve(ChatColors::Palette.size() + 1);

    // Group inheritance is distinct from the `default` color override.
    choices.push_back({tr.Get("color.groupDefault", viewerSlot), std::string{}});

    // The framework renders the palette; colors without a translation key fall back to their name.
    auto palette = ::VoltMod::BuildPaletteChoices([&](std::string_view name) -> std::string {
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

// Read the stored override, not the resolved inherited color.
static std::string CurrentSlotColor(App& app, int64_t steamId, ColorSlot slot)
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

static void AddColorChoice(App& app, MenuBuilder& builder, const std::string& title, int64_t steamId, ColorSlot slot,
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

// `lang.<code>` translations fall back to the raw code.
static std::string LanguageLabel(App& app, const std::string& code, int viewerSlot)
{
    std::string key = "lang." + code;
    std::string label = app.Runtime.Translations.Get(key, viewerSlot);
    return label == key ? code : label;
}

static std::vector<std::string> AvailableLanguagesSorted(App& app)
{
    auto langs = app.Runtime.Translations.GetAvailableLanguages();
    std::sort(langs.begin(), langs.end());
    return langs;
}

// Index zero represents no stored language override.
static int IndexForLanguage(const std::vector<std::string>& langs, const std::string& code)
{
    auto it = std::find(langs.begin(), langs.end(), code);
    return it != langs.end() ? static_cast<int>(it - langs.begin()) : 0;
}

static void AddLanguageChoice(App& app, MenuBuilder& builder, int64_t steamId, int viewerSlot)
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

std::shared_ptr<VoltMod::MenuView> BuildChatSettingsMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;
    auto* admin = app.Runtime.Players.GetPlayerBySlot(adminSlot);
    if (!admin)
        return nullptr;
    int64_t steamId = admin->GetSteamID();

    MenuBuilder builder(tr.Get("category.chatSettings", adminSlot));

    // Persist each row immediately; the menu has no Save action.
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

    AddColorChoice(app, builder, tr.Get("chat.nameColor", adminSlot), steamId, ColorSlot::Name, adminSlot);
    AddColorChoice(app, builder, tr.Get("chat.messageColor", adminSlot), steamId, ColorSlot::Message, adminSlot);

    AddLanguageChoice(app, builder, steamId, adminSlot);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
