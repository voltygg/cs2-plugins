#include "EffectAction.hpp"

#include "../../Core/Managers.hpp"

#include <CS2Kit/Core/Services.hpp>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Effects
{

void Run(int adminSlot, int targetSlot, const EffectToggle& effect)
{
    auto ctx = Actions::Resolve(adminSlot, targetSlot, effect.Flag);
    if (!ctx.Valid())
        return;

    bool on = App().Effects.Toggle(targetSlot, static_cast<int>(effect.Id), [&] { return effect.Enable(ctx); });
    Actions::Broadcast(ctx, on ? effect.OnKey : effect.OffKey);
}

}  // namespace AdminSystem::Admin::Effects
