#include "Admin/Menu/Tabs/MySettingsTab.hpp"

#include "Admin/AdminManager.hpp"
#include "Admin/Effects/Descriptors.hpp"
#include "Admin/Menu/MenuAccess.hpp"
#include "Admin/Menu/MenuCatalog.hpp"
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

/** Write one color back, leaving the admin's other chat settings as they are. */
static void SaveColor(App& app, int64_t steamId, ColorSlot slot, const std::string& value)
{
    auto& admins = app.Admins;
    const auto* admin = admins.GetAdmin(steamId);
    if (!admin)
        return;

    const std::string& name = slot == ColorSlot::Name ? value : admin->NameColor;
    const std::string& message = slot == ColorSlot::Message ? value : admin->MessageColor;
    admins.UpdateChatStyleAsync(steamId, admin->DisplayPrefix, name, message);
}

static VoltMod::MenuItem ColorRow(const MenuContext& ctx, std::string_view labelKey, ColorSlot slot)
{
    App& app = ctx.Plugin;
    const int64_t steamId = ctx.Admin.SteamId;

    return ChoiceRow<std::string>{
        .Label = ctx.Translate(labelKey),
        .Choices = BuildColorChoices(app, ctx.Admin.Slot),
        .Commit = [&app, steamId, slot](int, const std::string& value) { SaveColor(app, steamId, slot, value); },
        .Index = IndexForColor(CurrentSlotColor(app, steamId, slot))}
        .ToItem();
}

/** Hide acts on the admin alone, so it belongs with their own settings rather than among the rows
 *  that act on somebody else. */
static VoltMod::MenuItem HideRow(const MenuContext& ctx)
{
    App& app = ctx.Plugin;
    const VoltMod::PlayerRef admin = ctx.Admin;

    return ToggleRow{
        .Label = ctx.Translate("action.hide"),
        .Get = [&app, slot = admin.Slot](int) { return app.Effects.IsActive(slot, app.EffectDescriptors.Hide.Id); },
        .Flip = [&app, admin](int) { app.PlayerEffects.Toggle(admin, admin, app.EffectDescriptors.Hide); },
        .Enabled = Allows(app, Permission::Hide)}
        .ToItem();
}

static VoltMod::MenuItem PrefixRow(const MenuContext& ctx)
{
    App& app = ctx.Plugin;
    const int64_t steamId = ctx.Admin.SteamId;

    return ToggleRow{.Label = ctx.Translate("chat.displayPrefix"),
                     .Get =
                         [&app, steamId](int) {
                             const auto* admin = app.Admins.GetAdmin(steamId);
                             return admin ? admin->DisplayPrefix : true;
                         },
                     .Flip =
                         [&app, steamId](int) {
                             const auto* admin = app.Admins.GetAdmin(steamId);
                             if (admin)
                                 app.Admins.UpdateChatStyleAsync(steamId, !admin->DisplayPrefix, admin->NameColor,
                                                                 admin->MessageColor);
                         }}
        .ToItem();
}

std::shared_ptr<VoltMod::Menu> BuildMySettingsTab(const MenuContext& ctx)
{
    MenuBuilder builder(ctx.Translate("category.mySettings"));

    // Every row here persists immediately; the menu has no Save action.
    AppendCatalogRows(ctx, builder, MySettingsRows, [&](RowId id) -> VoltMod::MenuItem {
        switch (id)
        {
        case RowId::Hide:
            return HideRow(ctx);
        case RowId::ChatPrefix:
            return PrefixRow(ctx);
        case RowId::NameColor:
            return ColorRow(ctx, "chat.nameColor", ColorSlot::Name);
        case RowId::MessageColor:
            return ColorRow(ctx, "chat.messageColor", ColorSlot::Message);
        default:
            return {};
        }
    });

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
