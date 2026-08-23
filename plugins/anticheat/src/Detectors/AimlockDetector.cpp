#include "AimlockDetector.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Slot.hpp>
#include <charconv>
#include <string_view>

using CS2Kit::App::Engine;

namespace Anticheat
{

LagEstimate MeasureVisualLag(int slot)
{
    if (!CS2Kit::Core::IsValidSlot(slot) || !Engine().Sdk.NetChannels.GetNetInfo(slot))
        return {};

    const char* interp = Engine().Sdk.NetChannels.GetUserInfoCvar(slot, "cl_interp_ratio");
    if (!interp)
        return {};

    const std::string_view text(interp);
    float ratio = 0.0f;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), ratio);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
        return {};

    return EstimateVisualLag(Engine().Sdk.NetChannels.EngineLatency(slot), ratio);
}

}  // namespace Anticheat
