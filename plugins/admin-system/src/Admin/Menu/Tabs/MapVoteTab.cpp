#include "Admin/Menu/Tabs/MapVoteTab.hpp"

#include "Admin/Menu/MenuAccess.hpp"
#include "Admin/Menu/MenuCatalog.hpp"
#include "Admin/Menu/Pickers/MapPicker.hpp"
#include "Core/App.hpp"
#include "Core/ChatService.hpp"
#include "Core/Permissions.hpp"
#include "Maps/VoteState.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Runtime.hpp>
#include <utility>

namespace AdminSystem::Admin::Menu
{

using VoltMod::ButtonRow;
using VoltMod::MenuBuilder;
using VoltMod::SubmenuRow;

/** Opens the map list for @p verb. Verb first, so Cancel vote is not buried under the maps. */
static VoltMod::MenuItem MapListRow(const MenuContext& ctx, std::string_view labelKey, MapVerb verb,
                                    std::string_view permission)
{
    return SubmenuRow{.Label = ctx.Translate(labelKey),
                      .Build = [ctx, verb](int) { return BuildMapPicker(ctx, verb); },
                      .Enabled = Allows(ctx.Plugin, permission)}
        .ToItem();
}

/** Greys rather than disappears: a tab's rows are fixed when it opens, so a row added only while
 *  a vote ran would still be there once it ended. */
static VoltMod::MenuItem CancelVoteRow(const MenuContext& ctx)
{
    App& app = ctx.Plugin;

    VoltMod::MenuItem row = ButtonRow{.Label = ctx.Translate("action.cancelVote"),
                                      .Activate =
                                          [&app](int slot) {
                                              app.Votes.CancelVote();
                                              app.Chat.Reply(slot,
                                                             app.Runtime.Translations.Get("cmd.voteCancelled", slot));
                                          },
                                      .Enabled = Allows(app, Permission::Vote)}
                               .ToItem();

    return DisableUnless(std::move(row), [&app](int) { return app.Votes.IsRunning(); },
                         ctx.Translate("hint.noVote"));
}

std::shared_ptr<VoltMod::Menu> BuildMapVoteTab(const MenuContext& ctx)
{
    MenuBuilder builder(ctx.Translate("category.mapVote"));

    AppendCatalogRows(ctx, builder, MapVoteRows, [&](RowId id) -> VoltMod::MenuItem {
        switch (id)
        {
        case RowId::ChangeMap:
            return MapListRow(ctx, "action.changeMap", MapVerb::ChangeNow, Permission::Map);
        case RowId::SetNextMap:
            return MapListRow(ctx, "action.setNextMap", MapVerb::SetNext, Permission::Map);
        case RowId::VoteMap:
            return MapListRow(ctx, "action.voteMap", MapVerb::PutToVote, Permission::Vote);
        case RowId::CancelVote:
            return CancelVoteRow(ctx);
        default:
            return {};
        }
    });

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
