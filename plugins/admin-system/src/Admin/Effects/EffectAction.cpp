#include "EffectAction.hpp"
#include "../../Core/Managers.hpp"
#include <CS2Kit/Core/Services.hpp>

using CS2Kit::Core::Kit;

namespace AdminSystem::Admin::Effects
{

void Run(int adminSlot, int targetSlot, const EffectToggle& effect)
{
    auto ctx = Actions::Resolve(adminSlot, targetSlot, static_cast<char>(effect.Flag));
    if (!ctx.Valid())
        return;

    bool on = Sys().Effects.Toggle(targetSlot, effect.Id, [&] { return effect.Enable(ctx); });
    Actions::Broadcast(ctx, on ? effect.OnKey : effect.OffKey);
}

}  // namespace AdminSystem::Admin::Effects
