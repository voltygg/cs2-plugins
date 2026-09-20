#include "Admin/Menu/MenuAccess.hpp"

#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Entities/EntitySystem.hpp>
#include <VoltMod/Entities/Pawn.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <algorithm>
#include <utility>

namespace AdminSystem::Admin::Menu
{

bool Visible(App& app, int adminSlot, const RowSpec& row)
{
    if (!row.Children.empty())
        return AnyVisible(app, adminSlot, row.Children);
    if (row.Permission.empty())
        return app.Admins.IsAdmin(app.Runtime.Players.RefFor(adminSlot).SteamId);
    return MayUse(app, adminSlot, row.Permission);
}

bool AnyVisible(App& app, int adminSlot, std::span<const RowSpec> rows)
{
    return std::ranges::any_of(rows, [&](const RowSpec& row) { return Visible(app, adminSlot, row); });
}

VoltMod::MenuItem DisableUnless(VoltMod::MenuItem item, std::function<bool(int)> usable, std::string reason)
{
    auto describe = std::move(item.Describe);
    auto activate = std::move(item.Activate);
    auto step = std::move(item.Step);
    auto commit = std::move(item.Commit);

    item.Describe = [describe = std::move(describe), usable, reason = std::move(reason)](int slot) {
        VoltMod::MenuRow row = describe ? describe(slot) : VoltMod::MenuRow{};
        if (!row.Enabled || usable(slot))
            return row;
        row.Enabled = false;
        row.Value = reason;
        return row;
    };
    item.Activate = [activate = std::move(activate), usable](int slot, VoltMod::MenuSurface& surface) {
        if (activate && usable(slot))
            activate(slot, surface);
    };
    item.Step = [step = std::move(step), usable](int slot, int direction) {
        return step && usable(slot) && step(slot, direction);
    };
    item.Commit = [commit = std::move(commit), usable](int slot) {
        if (commit && usable(slot))
            commit(slot);
    };
    return item;
}

VoltMod::MenuItem WhileAlive(App& app, int adminSlot, VoltMod::PlayerRef target, VoltMod::MenuItem item)
{
    auto alive = [&entities = app.Runtime.Entities, target](int) {
        VoltMod::Pawn pawn = entities.PawnOf(target.Slot);
        return pawn && pawn.IsAlive();
    };
    return DisableUnless(std::move(item), alive, app.Runtime.Translations.Get("hint.targetDead", adminSlot));
}

VoltMod::MenuItem WhileTargetable(App& app, VoltMod::PlayerRef admin, VoltMod::PlayerRef target, VoltMod::MenuItem item)
{
    // An empty permission skips the permission check but still applies immunity and self-targeting.
    auto targetable = [&app, admin, target](int) {
        return app.Runtime.Policy.Authorize(admin, target, {}).has_value();
    };
    return DisableUnless(std::move(item), targetable, app.Runtime.Translations.Get("hint.immune", admin.Slot));
}

}  // namespace AdminSystem::Admin::Menu
