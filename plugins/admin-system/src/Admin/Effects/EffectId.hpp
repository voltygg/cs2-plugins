#pragma once

#include <cstdint>

namespace AdminSystem::Admin::Effects
{

// Stable per-effect id used as the key into the per-slot EffectManager map. Order is not
// load-bearing: the menu iterates the registered EffectEntry list, not this enum.
enum class EffectId : uint8_t
{
    Disco,
    Ghost,
    Hide,
    Wallhack,
    Model
};

}  // namespace AdminSystem::Admin::Effects
