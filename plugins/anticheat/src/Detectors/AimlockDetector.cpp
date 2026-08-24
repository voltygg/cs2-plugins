#include "AimlockDetector.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Slot.hpp>
#include <charconv>
#include <string_view>

namespace Anticheat
{

LagEstimate MeasureVisualLag(VoltMod::Runtime& rt, int slot)
{
    if (!VoltMod::Core::IsValidSlot(slot) || !rt.NetChannels.GetNetInfo(slot))
        return {};

    const char* interp = rt.NetChannels.GetUserInfoCvar(slot, "cl_interp_ratio");
    if (!interp)
        return {};

    const std::string_view text(interp);
    float ratio = 0.0f;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), ratio);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
        return {};

    return EstimateVisualLag(rt.NetChannels.EngineLatency(slot), ratio);
}

}  // namespace Anticheat
