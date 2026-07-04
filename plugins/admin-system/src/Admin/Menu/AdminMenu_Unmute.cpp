#include "AdminMenu_Unmute.hpp"

#include "../../Core/ChatService.hpp"
#include "../../Core/Managers.hpp"
#include "../../Core/Permissions.hpp"
#include "../AdminManager.hpp"
#include "Labels.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Menu/Flow.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <cstdint>
#include <format>
#include <optional>
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

void StartUnmuteConfirm(int adminSlot, MuteRow row)
{
    CS2Kit::Flow<MuteRow>::Create(std::move(row))
        // The Mute flag may have been revoked (e.g. !admin_reload) while the menu was open.
        ->OnValidate([](int slot, const MuteRow&) -> std::optional<std::string> {
            auto* admin = Engine().Players.GetPlayerBySlot(slot);
            if (!admin || !App().Admins.HasPermission(admin->GetSteamID(), Permission::Mute))
                return "punish.notAllowed";
            return std::nullopt;
        })
        ->WithConfirm(
            [](int slot) {
                auto& tr = Engine().Translations;
                return std::format("{}: {}", tr.Get("punish.confirmTitle", slot), tr.Get("action.unmute", slot));
            },
            [](int slot, const MuteRow& r) {
                auto& tr = Engine().Translations;
                std::vector<std::pair<std::string, std::string>> rows;
                rows.emplace_back(tr.Get("punish.target", slot), r.Name);
                rows.emplace_back(tr.Get(r.IsVoice ? "action.voiceMute" : "action.textMute", slot), "");
                rows.emplace_back(tr.Get("punish.duration", slot), ExpiryLabel(r.ExpiresAt, slot));
                rows.emplace_back(tr.Get("punish.reason", slot), StringUtils::TruncateUtf8(r.Reason, 40));
                return rows;
            },
            [](int slot) { return Engine().Translations.Get("punish.confirm", slot); },
            [](int slot) { return Engine().Translations.Get("punish.cancel", slot); })
        ->OnFinish([](int slot, MuteRow& r) {
            auto& tr = Engine().Translations;
            auto* admin = Engine().Players.GetPlayerBySlot(slot);
            if (!admin)
                return;

            // Remove* broadcasts the unmute; the extra reply covers broadcasts being disabled and
            // returns false when another server already lifted the mute.
            const bool removed =
                r.IsVoice
                    ? App().Punishments.RemoveVoiceMute(r.Id, admin->GetSteamID(), tr.Get("reason.voiceUnmutedByAdmin"))
                    : App().Punishments.RemoveTextMute(r.Id, admin->GetSteamID(), tr.Get("reason.textUnmutedByAdmin"));

            App().Chat.Reply(slot,
                             removed ? tr.Get("unmute.done", slot, {{"name", r.Name}}) : tr.Get("unmute.gone", slot));
        })
        ->Start(adminSlot);
}

/** Turn a cache snapshot of voice/text mutes into tagged menu rows. */
template <typename TMute>
void AppendMuteRows(MenuBuilder& builder, const std::vector<TMute>& mutes, bool isVoice, int adminSlot)
{
    auto& tr = Engine().Translations;
    for (const auto& mute : mutes)
    {
        MuteRow row{.Id = mute.Id,
                    .IsVoice = isVoice,
                    .Name = StringUtils::DisplayNameOr(mute.TargetSteamId, mute.TargetName),
                    .ExpiresAt = mute.ExpiresAt,
                    .Reason = mute.Reason};

        auto tag = tr.Get(isVoice ? "action.voiceMute" : "action.textMute", adminSlot);
        auto label = std::format("[{}] {} - {}", tag, row.Name, ExpiryLabel(row.ExpiresAt, adminSlot));
        builder.AddButton(label, [row = std::move(row)](int slot) { StartUnmuteConfirm(slot, row); });
    }
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
