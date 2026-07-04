#include "AdminMenu_Unmute.hpp"

#include "../../Core/ChatService.hpp"
#include "../../Core/Managers.hpp"
#include "../../Core/Permissions.hpp"
#include "../AdminManager.hpp"
#include "MenuHelpers.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Menu/MenuPresets.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <cstdint>
#include <format>
#include <string>
#include <utility>
#include <vector>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Menu
{

using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Utils::StringUtils;

namespace
{

/** The slice of a mute the menu needs; each row's lambda holds a copy, so keep it small. */
struct MuteRow
{
    int64_t Id = 0;
    bool IsVoice = true;
    std::string Name;
    int64_t ExpiresAt = 0;
    std::string Reason;
};

void ConfirmAndUnmute(int adminSlot, int64_t muteId, bool isVoice, const std::string& name)
{
    auto& tr = Engine().Translations;

    auto* admin = Engine().Players.GetPlayerBySlot(adminSlot);
    if (!admin)
        return;

    // The Mute flag may have been revoked (e.g. !admin_reload) while the menu was open.
    if (!App().Admins.HasPermission(admin->GetSteamID(), Permission::Mute))
    {
        App().Chat.Reply(adminSlot, tr.Get("punish.notAllowed", adminSlot));
        Engine().Menus.CloseAllMenus(adminSlot);
        return;
    }

    // Remove* broadcasts the unmute; the extra reply covers broadcasts being disabled and returns
    // false when another server already lifted the mute.
    const bool removed =
        isVoice ? App().Punishments.RemoveVoiceMute(muteId, admin->GetSteamID(), tr.Get("reason.voiceUnmutedByAdmin"))
                : App().Punishments.RemoveTextMute(muteId, admin->GetSteamID(), tr.Get("reason.textUnmutedByAdmin"));

    if (removed)
        App().Chat.Reply(adminSlot, tr.Get("unmute.done", adminSlot, {{"name", name}}));
    else
        App().Chat.Reply(adminSlot, tr.Get("unmute.gone", adminSlot));

    Engine().Menus.CloseAllMenus(adminSlot);
}

std::shared_ptr<CS2Kit::MenuView> BuildUnmuteConfirm(int adminSlot, const MuteRow& row)
{
    auto& tr = Engine().Translations;

    ::CS2Kit::Menu::ConfirmDialogSpec spec{
        .Title = std::format("{}: {}", tr.Get("punish.confirmTitle", adminSlot), tr.Get("action.unmute", adminSlot)),
        .ConfirmLabel = tr.Get("punish.confirm", adminSlot),
        .CancelLabel = tr.Get("punish.cancel", adminSlot),
        .OnConfirm = [id = row.Id, isVoice = row.IsVoice, name = row.Name](
                         int slot) { ConfirmAndUnmute(slot, id, isVoice, name); },
    };
    spec.BodyLines.push_back(std::format("{}: {}", tr.Get("punish.target", adminSlot), row.Name));
    spec.BodyLines.push_back(tr.Get(row.IsVoice ? "action.voiceMute" : "action.textMute", adminSlot));
    spec.BodyLines.push_back(
        std::format("{}: {}", tr.Get("punish.duration", adminSlot), ExpiryLabel(row.ExpiresAt, adminSlot)));
    spec.BodyLines.push_back(
        std::format("{}: {}", tr.Get("punish.reason", adminSlot), StringUtils::TruncateUtf8(row.Reason, 40)));

    return ::CS2Kit::Menu::BuildConfirmDialog(std::move(spec));
}

/** Append one selectable row per mute, tagged with its type; the row is moved into the button lambda. */
void AddMuteRow(MenuBuilder& builder, MuteRow row, int adminSlot)
{
    auto& tr = Engine().Translations;
    auto tag = tr.Get(row.IsVoice ? "action.voiceMute" : "action.textMute", adminSlot);
    auto label = std::format("[{}] {} — {}", tag, row.Name, ExpiryLabel(row.ExpiresAt, adminSlot));

    builder.AddButton(
        label, [row = std::move(row)](int slot) { Engine().Menus.OpenMenu(slot, BuildUnmuteConfirm(slot, row)); });
}

/** Turn a cache snapshot of voice/text mutes into tagged menu rows. */
template <typename TMute>
void AppendMuteRows(MenuBuilder& builder, const std::vector<TMute>& mutes, bool isVoice, int adminSlot)
{
    for (const auto& mute : mutes)
        AddMuteRow(builder,
                   MuteRow{.Id = mute.Id,
                           .IsVoice = isVoice,
                           .Name = MenuDisplayName(mute.TargetSteamId, mute.TargetName),
                           .ExpiresAt = mute.ExpiresAt,
                           .Reason = mute.Reason},
                   adminSlot);
}

}  // namespace

std::shared_ptr<CS2Kit::MenuView> BuildUnmuteMenu(int adminSlot)
{
    auto& tr = Engine().Translations;

    MenuBuilder builder(tr.Get("unmute.title", adminSlot));

    const auto voiceMutes = App().Punishments.GetActiveVoiceMutes();
    const auto textMutes = App().Punishments.GetActiveTextMutes();
    AppendMuteRows(builder, voiceMutes, true, adminSlot);
    AppendMuteRows(builder, textMutes, false, adminSlot);

    // Never show a dead-end empty page.
    if (voiceMutes.empty() && textMutes.empty())
        builder.AddButton(tr.Get("unmute.noMutes", adminSlot), [](int) {}, false);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
