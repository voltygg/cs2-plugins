#include "Admin/Menu/Tabs/MySettingsTab.hpp"

#include "Admin/AdminManager.hpp"
#include "Admin/Effects/Descriptors.hpp"
#include "Admin/Menu/MenuAccess.hpp"
#include "Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Menu/MenuPresets.hpp>
#include <VoltMod/Messaging/ChatColors.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace AdminSystem::Admin::Menu
{

using VoltMod::ChoiceRow;
using VoltMod::MenuBuilder;
using VoltMod::ToggleRow;

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

static std::vector<VoltMod::Labeled<std::string>> BuildColorChoices(App& app, int viewerSlot)
{
    auto& translations = app.Runtime.Translations;
    const auto& keys = ColorLabelKeys();

    std::vector<VoltMod::Labeled<std::string>> choices;
    choices.reserve(ChatColors::Palette.size() + 1);

    // Group inheritance is distinct from the `default` color override.
    choices.push_back({.Label = translations.Get("color.groupDefault", viewerSlot), .Value = ""});

    // The framework renders the palette; colors without a translation key fall back to their name.
    auto palette = ChatColors::PaletteChoices([&](std::string_view name) -> std::string {
        if (auto it = keys.find(name); it != keys.end())
            return translations.Get(std::string(it->second), viewerSlot);
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

    builder.Add(ChoiceRow<std::string>{.Label = title,
                                       .Choices = std::move(choices),
                                       .Commit =
                                           [&app, steamId, slot](int /*menuSlot*/, const std::string& value) {
                                               auto& admins = app.Admins;
                                               const auto* admin = admins.GetAdmin(steamId);
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
                                               admins.UpdateChatStyleAsync(steamId, admin->DisplayPrefix, nameColor,
                                                                           messageColor);
                                           },
                                       .Index = initialIndex});
}

std::shared_ptr<VoltMod::Menu> BuildMySettingsTab(const MenuContext& ctx)
{
    App& app = ctx.Plugin;
    const int adminSlot = ctx.Admin.Slot;
    const int64_t steamId = ctx.Admin.SteamId;

    MenuBuilder builder(ctx.Translate("category.mySettings"));

    // Hide acts on the admin alone, so it belongs with their own settings rather than among the
    // rows that act on somebody else.
    builder.Add(ToggleRow{
        .Label = ctx.Translate("action.hide"),
        .Get = [&app, adminSlot](int) { return app.Effects.IsActive(adminSlot, app.EffectDescriptors.Hide.Id); },
        .Flip =
            [&app, adminRef = ctx.Admin](int) {
                app.PlayerEffects.Toggle(adminRef, adminRef, app.EffectDescriptors.Hide);
            },
        .Enabled = Allows(app, Permission::Hide)});

    // Persist each row immediately; the menu has no Save action.
    builder.Add(ToggleRow{.Label = ctx.Translate("chat.displayPrefix"),
                          .Get =
                              [&app, steamId](int) {
                                  const auto* a = app.Admins.GetAdmin(steamId);
                                  return a ? a->DisplayPrefix : true;
                              },
                          .Flip =
                              [&app, steamId](int) {
                                  auto& admins = app.Admins;
                                  const auto* a = admins.GetAdmin(steamId);
                                  if (!a)
                                      return;
                                  admins.UpdateChatStyleAsync(steamId, !a->DisplayPrefix, a->NameColor,
                                                              a->MessageColor);
                              }});

    AddColorChoice(app, builder, ctx.Translate("chat.nameColor"), steamId, ColorSlot::Name, adminSlot);
    AddColorChoice(app, builder, ctx.Translate("chat.messageColor"), steamId, ColorSlot::Message, adminSlot);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
