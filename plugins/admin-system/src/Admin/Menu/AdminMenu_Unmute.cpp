#include "AdminMenu_Unmute.hpp"

#include "../../Core/App.hpp"
#include "../../Core/ChatService.hpp"
#include "../../Core/Permissions.hpp"
#include "../AdminManager.hpp"
#include "Labels.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/StringUtils.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/Flow.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Runtime.hpp>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace AdminSystem::Admin::Menu
{

using VoltMod::Core::StringUtils;
using VoltMod::Menu::MenuBuilder;

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

void StartUnmuteConfirm(App& app, int adminSlot, MuteRow row)
{
    VoltMod::Flow<MuteRow>::Create(app.Runtime.Menus, std::move(row))
        // The Mute flag may have been revoked (e.g. !admin_reload) while the menu was open.
        ->OnValidate([&app](int slot, const MuteRow&) -> std::optional<std::string> {
            auto* admin = app.Runtime.Players.GetPlayerBySlot(slot);
            if (!admin || !app.Access.HasPermission(admin->GetSteamID(), Permission::Mute))
                return "punish.notAllowed";
            return std::nullopt;
        })
        ->WithConfirm(
            [&app](int slot) {
                auto& tr = app.Runtime.Translations;
                return std::format("{}: {}", tr.Get("punish.confirmTitle", slot), tr.Get("action.unmute", slot));
            },
            [&app](int slot, const MuteRow& r) {
                auto& tr = app.Runtime.Translations;
                std::vector<std::pair<std::string, std::string>> rows;
                rows.emplace_back(tr.Get("punish.target", slot), r.Name);
                rows.emplace_back(tr.Get(r.IsVoice ? "action.voiceMute" : "action.textMute", slot), "");
                rows.emplace_back(tr.Get("punish.duration", slot), ExpiryLabel(tr, r.ExpiresAt, slot));
                rows.emplace_back(tr.Get("punish.reason", slot), StringUtils::TruncateUtf8(r.Reason, 40));
                return rows;
            },
            [&app](int slot) { return app.Runtime.Translations.Get("punish.confirm", slot); },
            [&app](int slot) { return app.Runtime.Translations.Get("punish.cancel", slot); })
        ->OnFinish([&app](int slot, MuteRow& r) {
            auto& tr = app.Runtime.Translations;
            auto* admin = app.Runtime.Players.GetPlayerBySlot(slot);
            if (!admin)
                return;

            // Remove* broadcasts the unmute; the extra reply covers broadcasts being disabled and
            // returns false when another server already lifted the mute.
            const bool removed =
                r.IsVoice
                    ? app.Punishments.RemoveVoiceMute(r.Id, admin->GetSteamID(), tr.Get("reason.voiceUnmutedByAdmin"))
                    : app.Punishments.RemoveTextMute(r.Id, admin->GetSteamID(), tr.Get("reason.textUnmutedByAdmin"));

            app.Chat.Reply(slot,
                           removed ? tr.Get("unmute.done", slot, {{"name", r.Name}}) : tr.Get("unmute.gone", slot));
        })
        ->Start(adminSlot);
}

/** Turn a cache snapshot of voice/text mutes into tagged menu rows. */
template <typename TMute>
void AppendMuteRows(App& app, MenuBuilder& builder, const std::vector<TMute>& mutes, bool isVoice, int adminSlot)
{
    auto& tr = app.Runtime.Translations;
    for (const auto& mute : mutes)
    {
        MuteRow row{.Id = mute.Id,
                    .IsVoice = isVoice,
                    .Name = StringUtils::DisplayNameOr(mute.TargetSteamId, mute.TargetName),
                    .ExpiresAt = mute.ExpiresAt,
                    .Reason = mute.Reason};

        auto tag = tr.Get(isVoice ? "action.voiceMute" : "action.textMute", adminSlot);
        auto label = std::format("[{}] {} - {}", tag, row.Name, ExpiryLabel(tr, row.ExpiresAt, adminSlot));
        builder.AddButton(label, [&app, row = std::move(row)](int slot) { StartUnmuteConfirm(app, slot, row); });
    }
}

}  // namespace

std::shared_ptr<VoltMod::MenuView> BuildUnmuteMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;

    MenuBuilder builder(tr.Get("unmute.title", adminSlot));

    const auto voiceMutes = app.Punishments.GetActiveVoiceMutes();
    const auto textMutes = app.Punishments.GetActiveTextMutes();
    AppendMuteRows(app, builder, voiceMutes, true, adminSlot);
    AppendMuteRows(app, builder, textMutes, false, adminSlot);

    // Never show a dead-end empty page.
    if (voiceMutes.empty() && textMutes.empty())
        builder.AddButton(tr.Get("unmute.noMutes", adminSlot), [](int) {}, false);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
