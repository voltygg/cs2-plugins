#include "Admin/Menu/Flows/LiftFlow.hpp"

#include "Admin/Menu/Labels.hpp"
#include "Admin/Menu/MenuAccess.hpp"
#include "Admin/Menu/MenuCatalog.hpp"
#include "Core/App.hpp"
#include "Core/ChatService.hpp"
#include "Punishments/PunishType.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Text/Strings.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Menu/Flow.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Runtime.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <span>
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

/** Translation key of the punishment tag. Bans and mutes share one list, so every row is tagged. */
static std::string_view TagKey(PunishType kind)
{
    return kind == PunishType::Ban ? std::string_view{"action.ban"} : ActionTranslationKey(kind);
}

/** Lift the punishment; false when another server already did. */
static bool Lift(App& app, const LiftRow& row, int64_t adminSteamId)
{
    const std::string_view reasonKey = LiftReasonKey(row.Kind);
    if (reasonKey.empty())
        return false;  // Not liftable: no row is ever built for these.

    return app.Punishments.Remove(row.Kind, row.Id, adminSteamId, app.Runtime.Translations.Get(reasonKey));
}

static void StartLiftConfirm(App& app, int adminSlot, LiftRow row)
{
    const bool ban = row.Kind == PunishType::Ban;
    const RowSpec& kind = ban ? LiftBansRow : LiftMutesRow;
    const std::string_view done = ban ? "unban.done" : "unmute.done";
    const std::string_view gone = ban ? "unban.gone" : "unmute.gone";

    auto& translations = app.Runtime.Translations;

    VoltMod::Flow<LiftRow>::Create(app.Runtime.Menus, adminSlot, std::move(row))
        ->Validate(RequirePermission(app, kind.Permission, adminSlot))
        ->Confirm({.Title = ConfirmTitle(translations, kind.LabelKey, adminSlot),
                   .Summary =
                       [&app, adminSlot](const LiftRow& r, VoltMod::SummaryRows& rows) {
                           auto& translations = app.Runtime.Translations;
                           rows.Add(translations.Get("punish.target", adminSlot), r.Name)
                               .Add(translations.Get(TagKey(r.Kind), adminSlot))
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
        ->Begin();
}

/** How many punishments the lift list draws. A busy server holds thousands, which no menu can
 *  usefully page through; the rest are lifted by command. */
static constexpr std::size_t ListLimit = 30;

/** One row per punishment, tagged with its kind. */
static void AppendRows(App& app, MenuBuilder& builder, const std::vector<Database::Punishment>& punishments,
                       int adminSlot)
{
    auto& translations = app.Runtime.Translations;
    for (const auto& punishment : punishments)
    {
        LiftRow row{.Kind = punishment.Kind,
                    .Id = punishment.Id,
                    .Name = Strings::DisplayNameOr(punishment.TargetSteamId, punishment.TargetName),
                    .ExpiresAt = punishment.ExpiresAt,
                    .Reason = punishment.Reason};

        auto label = std::format("[{}] {} - {}", translations.Get(TagKey(row.Kind), adminSlot), row.Name,
                                 ExpiryLabel(translations, row.ExpiresAt, adminSlot));
        builder.Button(label, [&app, row = std::move(row)](int slot) { StartLiftConfirm(app, slot, row); });
    }
}

std::shared_ptr<VoltMod::Menu> BuildLiftMenu(const MenuContext& ctx)
{
    App& app = ctx.Plugin;
    const int adminSlot = ctx.Admin.Slot;

    MenuBuilder builder(ctx.Translate(PunishLiftList.LabelKey));
    builder.EmptyText(ctx.Translate("lift.empty"));

    // One list, but each kind still appears only for an admin who may lift it.
    std::array<PunishType, 3> kinds{};
    std::size_t count = 0;
    if (ctx.Visible(LiftBansRow))
        kinds[count++] = PunishType::Ban;
    if (ctx.Visible(LiftMutesRow))
    {
        kinds[count++] = PunishType::VoiceMute;
        kinds[count++] = PunishType::TextMute;
    }

    const auto page = app.Punishments.GetActive(std::span{kinds}.first(count), ListLimit);
    AppendRows(app, builder, page.Rows, adminSlot);

    // Say so rather than pretending the list is everything; the rest are lifted by command.
    if (page.Total > page.Rows.size())
    {
        builder.Text(ctx.Translate(
            "lift.truncated", {{"shown", std::to_string(page.Rows.size())}, {"total", std::to_string(page.Total)}}));
    }

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
