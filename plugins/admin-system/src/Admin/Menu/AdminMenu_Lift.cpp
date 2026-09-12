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

using VoltMod::ButtonRow;
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
    std::string_view reasonKey;
    switch (row.Kind)
    {
    case PunishType::Ban:
        reasonKey = "reason.unbannedByAdmin";
        break;
    case PunishType::VoiceMute:
        reasonKey = "reason.voiceUnmutedByAdmin";
        break;
    case PunishType::TextMute:
        reasonKey = "reason.textUnmutedByAdmin";
        break;
    case PunishType::Kick:
    case PunishType::Warn:
        return false;  // Not liftable: no row is ever built for these.
    }

    return app.Punishments.Remove(row.Kind, row.Id, adminSteamId,
                                  app.Runtime.Translations.Get(std::string(reasonKey)));
}

static void StartLiftConfirm(App& app, int adminSlot, LiftRow row)
{
    const bool ban = row.Kind == PunishType::Ban;
    const Permission permission = ban ? Permission::Unban : Permission::Mute;
    const std::string_view action = ban ? "action.unban" : "action.unmute";
    const std::string_view done = ban ? "unban.done" : "unmute.done";
    const std::string_view gone = ban ? "unban.gone" : "unmute.gone";

    auto& translations = app.Runtime.Translations;

    VoltMod::Flow<LiftRow>::Create(app.MenuFor(adminSlot), adminSlot, std::move(row))
        ->Validate(RequirePermission(app, permission, adminSlot))
        ->Confirm({.Title = ConfirmTitle(translations, action, adminSlot),
                   .Summary =
                       [&app, adminSlot](const LiftRow& r, VoltMod::SummaryRows& rows) {
                           auto& translations = app.Runtime.Translations;
                           const auto tag = TagKey(r.Kind);
                           rows.Add(translations.Get("punish.target", adminSlot), r.Name)
                               .AddIf(!tag.empty(), translations.Get(tag, adminSlot))
                               .Add(translations.Get("punish.duration", adminSlot),
                                    ExpiryLabel(translations, r.ExpiresAt, adminSlot))
                               .Add(translations.Get("punish.reason", adminSlot), Strings::TruncateUtf8(r.Reason, 40));
                       }})
        ->Finish([&app, adminSlot, done, gone](LiftRow& r) {
            auto& translations = app.Runtime.Translations;
            auto* admin = app.Runtime.Players.Get(adminSlot);
            if (!admin)
                return;

            // Lift broadcasts the removal; the reply covers broadcasts being disabled.
            const bool removed = Lift(app, r, admin->SteamId());
            app.Chat.Reply(adminSlot, removed ? translations.Get(done, adminSlot, {{"name", r.Name}})
                                              : translations.Get(gone, adminSlot));
        })
        ->Start();
}

/** One row per punishment. Bans carry no tag; mutes are tagged with their kind. */
static void AppendRows(App& app, MenuBuilder& builder, const std::vector<Database::Punishment>& punishments,
                       PunishType kind, int adminSlot)
{
    auto& translations = app.Runtime.Translations;
    for (const auto& punishment : punishments)
    {
        LiftRow row{.Kind = kind,
                    .Id = punishment.Id,
                    .Name = Strings::DisplayNameOr(punishment.TargetSteamId, punishment.TargetName),
                    .ExpiresAt = punishment.ExpiresAt,
                    .Reason = punishment.Reason};

        const auto tag = TagKey(kind);
        const std::string prefix = tag.empty() ? "" : std::format("[{}] ", translations.Get(tag, adminSlot));
        auto label = std::format("{}{} - {}", prefix, row.Name, ExpiryLabel(translations, row.ExpiresAt, adminSlot));
        builder.Button(label, [&app, row = std::move(row)](int slot) { StartLiftConfirm(app, slot, row); });
    }
}

std::shared_ptr<VoltMod::Menu> BuildUnbanMenu(AdminSystem::App& app, int adminSlot)
{
    auto& translations = app.Runtime.Translations;
    MenuBuilder builder(translations.Get("unban.title", adminSlot));

    builder.EmptyText(translations.Get("unban.noBans", adminSlot));
    AppendRows(app, builder, app.Punishments.GetActive(PunishType::Ban), PunishType::Ban, adminSlot);

    return builder.Build();
}

std::shared_ptr<VoltMod::Menu> BuildUnmuteMenu(AdminSystem::App& app, int adminSlot)
{
    auto& translations = app.Runtime.Translations;
    MenuBuilder builder(translations.Get("unmute.title", adminSlot));

    builder.EmptyText(translations.Get("unmute.noMutes", adminSlot));
    AppendRows(app, builder, app.Punishments.GetActive(PunishType::VoiceMute), PunishType::VoiceMute, adminSlot);
    AppendRows(app, builder, app.Punishments.GetActive(PunishType::TextMute), PunishType::TextMute, adminSlot);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
