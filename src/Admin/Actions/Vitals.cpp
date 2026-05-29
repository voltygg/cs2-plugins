#include "Vitals.hpp"

#include "ActionContext.hpp"

#include <CS2Kit/Sdk/Entity.hpp>

namespace AdminSystem::Admin::Actions
{

using OptKey = std::optional<std::string>;

void DoSlay(int adminSlot, int targetSlot)
{
    RunAction(adminSlot, targetSlot, 's', [](const ActionContext& ctx) -> OptKey {
        if (!ctx.TargetCtrl.IsAlive())
            return std::nullopt;
        ctx.TargetCtrl.Slay();
        return "broadcast.slain";
    });
}

void DoSetHealth(int adminSlot, int targetSlot, int health)
{
    RunAction(adminSlot, targetSlot, 'h', [health](const ActionContext& ctx) -> OptKey {
        if (!ctx.TargetCtrl.IsAlive())
            return std::nullopt;
        ctx.TargetCtrl.SetHealth(health);
        return "broadcast.healed";
    });
}

void DoSetArmor(int adminSlot, int targetSlot, int armor)
{
    RunAction(adminSlot, targetSlot, 'h', [armor](const ActionContext& ctx) -> OptKey {
        if (!ctx.TargetCtrl.IsAlive())
            return std::nullopt;
        ctx.TargetCtrl.SetArmor(armor);
        return "broadcast.armored";
    });
}

void DoToggleGodmode(int adminSlot, int targetSlot)
{
    RunAction(adminSlot, targetSlot, 'h', [](const ActionContext& ctx) -> OptKey {
        if (!ctx.TargetCtrl.IsAlive())
            return std::nullopt;
        uint32_t flags = ctx.TargetCtrl.GetFlags();
        bool turningOn = (flags & CS2Kit::Sdk::FL_GODMODE) == 0;
        ctx.TargetCtrl.SetFlags(turningOn ? (flags | CS2Kit::Sdk::FL_GODMODE) : (flags & ~CS2Kit::Sdk::FL_GODMODE));
        return turningOn ? "broadcast.godmodeOn" : "broadcast.godmodeOff";
    });
}

}  // namespace AdminSystem::Admin::Actions
