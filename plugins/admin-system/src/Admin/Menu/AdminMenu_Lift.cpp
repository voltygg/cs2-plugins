#include "AdminMenu_Lift.hpp"

#include "../../Core/App.hpp"
#include "../../Core/ChatService.hpp"
#include "../../Core/Permissions.hpp"
#include "../../Punishments/PunishType.hpp"
#include "Labels.hpp"
#include "MenuAccess.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Strings.hpp>
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

using AdminSystem::Punishments::ActionTranslationKey;
using AdminSystem::Punishments::PunishType;

namespace AdminSystem::Admin::Menu
{

using VoltMod::MenuBuilder;
using VoltMod::Strings;

/** The slice of a punishment the menu needs; each row's lambda holds a copy, so keep it small. */
struct LiftRow
{
    PunishType Kind = PunishType::Ban;
    int64_t Id = 0;
    std::string Name;
    int64_t ExpiresAt = 0;
    std::string Reason;
};

/** Translation key of the punishment tag for a kind, or empty for a ban (which has no tag). */
static std::string_view TagKey(PunishType kind)
{
    return kind == PunishType::Ban ? std::string_view{} : ActionTranslationKey(kind);
}

/** Lift the punishment; false when another server already did. */
static bool Lift(App& app, const LiftRow& row, int64_t adminSteamId)
{
    auto& tr = app.Runtime.Translations;
    auto& punishments = app.Punishments;
    switch (row.Kind)
    {
    case PunishType::Ban:
        return punishments.RemoveBan(row.Id, adminSteamId, tr.Get("reason.unbannedByAdmin"));
    case PunishType::VoiceMute:
        return punishments.RemoveVoiceMute(row.Id, adminSteamId, tr.Get("reason.voiceUnmutedByAdmin"));
    case PunishType::TextMute:
        return punishments.RemoveTextMute(row.Id, adminSteamId, tr.Get("reason.textUnmutedByAdmin"));
    case PunishType::Kick:
    case PunishType::Warn:
        break;  // Not liftable: no row is ever built for these.
    }
    return false;
}

static void StartLiftConfirm(App& app, int adminSlot, LiftRow row)
{
    const bool ban = row.Kind == PunishType::Ban;
    const Permission permission = ban ? Permission::Unban : Permission::Mute;
    const std::string_view action = ban ? "action.unban" : "action.unmute";
    const std::string_view done = ban ? "unban.done" : "unmute.done";
    const std::string_view gone = ban ? "unban.gone" : "unmute.gone";

    VoltMod::Flow<LiftRow>::Create(app.Menus(), std::move(row))
        ->OnValidate(RequirePermission(app, permission))
        ->WithConfirm([&app, action](int slot) { return ConfirmTitle(app.Runtime.Translations, action, slot); },
                      [&app](int slot, const LiftRow& r) {
                          auto& tr = app.Runtime.Translations;
                          std::vector<std::pair<std::string, std::string>> rows;
                          rows.emplace_back(tr.Get("punish.target", slot), r.Name);
                          if (const auto tag = TagKey(r.Kind); !tag.empty())
                              rows.emplace_back(tr.Get(tag, slot), "");
                          rows.emplace_back(tr.Get("punish.duration", slot), ExpiryLabel(tr, r.ExpiresAt, slot));
                          rows.emplace_back(tr.Get("punish.reason", slot), Strings::TruncateUtf8(r.Reason, 40));
                          return rows;
                      },
                      ConfirmLabel(app.Runtime.Translations), CancelLabel(app.Runtime.Translations))
        ->OnFinish([&app, done, gone](int slot, LiftRow& r) {
            auto& tr = app.Runtime.Translations;
            auto* admin = app.Runtime.Players.Get(slot);
            if (!admin)
                return;

            // Lift broadcasts the removal; the reply covers broadcasts being disabled.
            const bool removed = Lift(app, r, admin->SteamId());
            app.Chat.Reply(slot, removed ? tr.Get(done, slot, {{"name", r.Name}}) : tr.Get(gone, slot));
        })
        ->Start(adminSlot);
}

/** One row per punishment. Bans carry no tag; mutes are tagged with their kind. */
template <typename TPunishment>
static void AppendRows(App& app, MenuBuilder& builder, const std::vector<TPunishment>& punishments, PunishType kind,
                       int adminSlot)
{
    auto& tr = app.Runtime.Translations;
    for (const auto& punishment : punishments)
    {
        LiftRow row{.Kind = kind,
                    .Id = punishment.Id,
                    .Name = Strings::DisplayNameOr(punishment.TargetSteamId, punishment.TargetName),
                    .ExpiresAt = punishment.ExpiresAt,
                    .Reason = punishment.Reason};

        const auto tag = TagKey(kind);
        const std::string prefix = tag.empty() ? "" : std::format("[{}] ", tr.Get(tag, adminSlot));
        auto label = std::format("{}{} - {}", prefix, row.Name, ExpiryLabel(tr, row.ExpiresAt, adminSlot));
        builder.Button(label, [&app, row = std::move(row)](int slot) { StartLiftConfirm(app, slot, row); });
    }
}

std::shared_ptr<VoltMod::MenuView> BuildUnbanMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;
    MenuBuilder builder(tr.Get("unban.title", adminSlot));

    const auto bans = app.Punishments.GetActiveBans();
    AppendRows(app, builder, bans, PunishType::Ban, adminSlot);

    // Never show a dead-end empty page.
    if (bans.empty())
        builder.Button(tr.Get("unban.noBans", adminSlot), [](int) {}, false);

    return builder.Build();
}

std::shared_ptr<VoltMod::MenuView> BuildUnmuteMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;
    MenuBuilder builder(tr.Get("unmute.title", adminSlot));

    const auto voiceMutes = app.Punishments.GetActiveVoiceMutes();
    const auto textMutes = app.Punishments.GetActiveTextMutes();
    AppendRows(app, builder, voiceMutes, PunishType::VoiceMute, adminSlot);
    AppendRows(app, builder, textMutes, PunishType::TextMute, adminSlot);

    if (voiceMutes.empty() && textMutes.empty())
        builder.Button(tr.Get("unmute.noMutes", adminSlot), [](int) {}, false);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
