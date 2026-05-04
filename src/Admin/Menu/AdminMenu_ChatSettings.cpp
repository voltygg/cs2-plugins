#include "AdminMenu_ChatSettings.hpp"

#include "../AdminManager.hpp"

#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/ChatColors.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <string>
#include <string_view>
#include <unordered_map>

namespace AdminSystem::Admin::Menu
{

using CS2Kit::Menu::ChoiceOption;
using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Players::PlayerManager;
using CS2Kit::Utils::Translations;

namespace
{

namespace ChatColors = CS2Kit::Utils::ChatColors;

// Translation-key suffix per canonical CS2Kit color name. Compound names ("lightblue")
// don't survive a generic "uppercase first letter" rule, so the small lookup keeps the
// JSON keys stable and lets us reuse the kit's @ref ChatColors::Palette for everything else.
const std::unordered_map<std::string_view, std::string_view>& ColorLabelKeys()
{
    static const std::unordered_map<std::string_view, std::string_view> kKeys = {
        {"default", "colorDefault"}, {"darkred", "colorDarkRed"},     {"lightpurple", "colorLightPurple"},
        {"green", "colorGreen"},     {"olive", "colorOlive"},         {"lime", "colorLime"},
        {"red", "colorRed"},         {"gray", "colorGray"},           {"yellow", "colorYellow"},
        {"silver", "colorSilver"},   {"lightblue", "colorLightBlue"}, {"darkblue", "colorDarkBlue"},
        {"purple", "colorPurple"},   {"lightred", "colorLightRed"},   {"gold", "colorGold"},
    };
    return kKeys;
}

// The DB stores empty string when the admin wants their group default; index 0 in the
// rendered choice list maps to that empty value. We then append every CS2Kit canonical
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

std::vector<ChoiceOption<std::string>::Choice> BuildColorChoices()
{
    auto& tr = Translations::Instance();
    const auto& keys = ColorLabelKeys();

    std::vector<ChoiceOption<std::string>::Choice> choices;
    choices.reserve(ChatColors::Palette.size() + 1);

    // First entry is always "(group default)" — empty DB value, distinct from
    // ChatColors::Default which is itself a real overrideable color.
    choices.push_back({tr.Get("colorGroupDefault"), std::string{}});

    for (const auto& entry : ChatColors::Palette)
    {
        std::string label;
        if (auto it = keys.find(entry.Name); it != keys.end())
            label = tr.Get(std::string(it->second));
        else
            label = std::string(entry.Name);  // Fallback if a new color lacks a translation key.
        choices.push_back({std::move(label), std::string(entry.Name)});
    }
    return choices;
}

enum class ColorSlot
{
    Name,
    Message,
};

// Pull the current persisted color out of the admin row (NOT the resolved style — we
// want the override slot itself, so "default" stays selected after a rebroadcast).
std::string CurrentSlotColor(int64_t steamId, ColorSlot slot)
{
    const auto* admin = AdminManager::Instance().GetAdmin(steamId);
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

void AddColorChoice(MenuBuilder& builder, const std::string& title, int64_t steamId, ColorSlot slot,
                    std::shared_ptr<int> pendingIdx)
{
    auto choices = BuildColorChoices();
    *pendingIdx = IndexForColor(CurrentSlotColor(steamId, slot));

    builder.AddChoice<std::string>(
        title, std::move(choices), [pendingIdx](int) { return *pendingIdx; },
        [pendingIdx](int, int newIdx) { *pendingIdx = newIdx; },
        [steamId, slot](int /*menuSlot*/, const std::string& value) {
            auto& mgr = AdminManager::Instance();
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
        });
}

}  // namespace

std::shared_ptr<::CS2Kit::Menu::Menu> BuildChatSettingsMenu(int adminSlot)
{
    auto& tr = Translations::Instance();
    auto* admin = PlayerManager::Instance().GetPlayerBySlot(adminSlot);
    if (!admin)
        return nullptr;
    int64_t steamId = admin->GetSteamID();

    MenuBuilder builder(tr.Get("categoryChatSettings"));

    // Display Prefix toggle — flips the bool and persists immediately so the next chat line
    // reflects the change without needing an explicit "Save" row.
    builder.AddToggle(
        tr.Get("chatDisplayPrefix"), tr.Get("effectStateOn"), tr.Get("effectStateOff"),
        [steamId](int) {
            const auto* a = AdminManager::Instance().GetAdmin(steamId);
            return a ? a->DisplayPrefix : true;
        },
        [steamId](int) {
            auto& mgr = AdminManager::Instance();
            const auto* a = mgr.GetAdmin(steamId);
            if (!a)
                return;
            mgr.UpdateChatStyle(steamId, !a->DisplayPrefix, a->NameColor, a->MessageColor);
        });

    // Each color picker keeps its own pending index; commit on E persists that single slot.
    AddColorChoice(builder, tr.Get("chatNameColor"), steamId, ColorSlot::Name, std::make_shared<int>(0));
    AddColorChoice(builder, tr.Get("chatMessageColor"), steamId, ColorSlot::Message, std::make_shared<int>(0));

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
