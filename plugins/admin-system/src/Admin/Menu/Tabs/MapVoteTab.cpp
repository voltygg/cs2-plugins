#include "Admin/Menu/Tabs/MapVoteTab.hpp"

#include "Admin/Menu/MenuAccess.hpp"
#include "Admin/Menu/MenuCatalog.hpp"
#include "Admin/Menu/Pickers/MapPicker.hpp"
#include "App.hpp"
#include "Core/ChatService.hpp"
#include "Core/Permissions.hpp"
#include "Maps/VoteState.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Runtime.hpp>
#include <string>
#include <utility>

namespace AdminSystem::Admin::Menu
{

using VoltMod::ButtonRow;
using VoltMod::MenuBuilder;
using VoltMod::SubmenuRow;

/** Opens the map list for @p verb. Verb first, so Cancel vote is not buried under the maps. */
static VoltMod::MenuItem MapListRow(const MenuContext& ctx, const RowSpec& spec, MapVerb verb)
{
    const std::string title = ctx.Translate(spec.LabelKey);
    return SubmenuRow{.Label = title,
                      .Build = [ctx, verb, title](int) { return BuildMapPicker(ctx, verb, title); },
                      .Enabled = Allows(ctx.Plugin, spec.Permission)}
        .ToItem();
}

/** Greys rather than disappears: a tab's rows are fixed when it opens, so a row added only while
 *  a vote ran would still be there once it ended. */
static VoltMod::MenuItem CancelVoteRow(const MenuContext& ctx, const RowSpec& spec)
{
    App& app = ctx.Plugin;

    VoltMod::MenuItem row =
        ButtonRow{.Label = ctx.Translate(spec.LabelKey),
                  .Activate =
                      [&app](int slot) {
                          app.Votes.CancelVote();
                          app.Chat.Reply(slot, app.Runtime.Translations.Get("cmd.voteCancelled", slot));
                      },
                  .Enabled = Allows(app, spec.Permission)}
            .ToItem();

    return DisableUnless(std::move(row), [&app](int) { return app.Votes.IsRunning(); }, ctx.Translate("hint.noVote"));
}

std::shared_ptr<VoltMod::Menu> BuildMapVoteTab(const MenuContext& ctx)
{
    MenuBuilder builder(ctx.Translate("category.mapVote"));

    AppendCatalogRows(ctx, builder, MapVoteRows, [&](const RowSpec& spec) -> VoltMod::MenuItem {
        switch (spec.Id)
        {
        case RowId::ChangeMap:
            return MapListRow(ctx, spec, MapVerb::ChangeNow);
        case RowId::SetNextMap:
            return MapListRow(ctx, spec, MapVerb::SetNext);
        case RowId::VoteMap:
            return MapListRow(ctx, spec, MapVerb::PutToVote);
        case RowId::CancelVote:
            return CancelVoteRow(ctx, spec);
        default:
            return {};
        }
    });

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
