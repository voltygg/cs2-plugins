#pragma once

#include <cstdint>

namespace AdminSystem::Admin::Effects
{

enum class EffectId : uint8_t
{
    Disco = 0,
    Ghost = 1,
    Hide = 2,
    Wallhack = 3,
    // Parameterized (not an EffectToggle); no Descriptors entry.
    Model = 4,

    // Sentinel = effect count; bounds-checks the descriptor registry (Descriptors.hpp). Never dispatch.
    Count
};

}  // namespace AdminSystem::Admin::Effects
