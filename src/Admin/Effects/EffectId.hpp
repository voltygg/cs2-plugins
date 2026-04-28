#pragma once

#include <cstdint>

namespace AdminSystem::Admin::Effects
{

enum class EffectId : uint8_t
{
    Disco = 0,
    Ghost = 1,
    Hide = 2,
    Blind = 3,

    Count
};

}  // namespace AdminSystem::Admin::Effects
