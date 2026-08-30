#include "DiscordReporter.hpp"

#include "App.hpp"

#include <VoltMod/Core/Json.hpp>
#include <VoltMod/Core/Log.hpp>
#include <string>
#include <string_view>
#include <utility>

namespace Log = VoltMod::Log;

namespace Anticheat
{

static constexpr long RequestTimeoutMs = 5000;
static constexpr int EmbedColor = 0xE04F4F;

void DiscordReporter::Report(int slot, const std::string& playerName, int64_t steamId, const Finding& finding,
                             FunnelOutcome outcome)
{
    const auto& settings = _config.Get().anticheat;
    if (settings.webhook.url.empty())
        return;
    if (!_throttle.TryAcquire({steamId, static_cast<int>(finding.Kind)}, VoltMod::Time::Now()))
        return;

    const auto field = [](std::string_view name, auto&& value, bool inlined = false) {
        return glz::obj{"name", name, "value", std::forward<decltype(value)>(value), "inline", inlined};
    };
    const auto payload = glz::obj{
        "embeds",
        glz::arr{glz::obj{
            "title", DisplayName(finding.Kind), "color", EmbedColor, "fields",
            glz::arr{field("Player", playerName.empty() ? std::string("<unknown>") : playerName, true),
                     field("SteamID64", steamId ? std::to_string(steamId) : std::string("unavailable"), true),
                     field("Slot", std::to_string(slot), true), field("Outcome", OutcomeName(outcome), true),
                     field("Mode", ModeName(ParseMode(settings.mode)), true),
                     field("Evidence", finding.Evidence.empty() ? std::string("-") : finding.Evidence)}}}};

    _rt.Http.Post(
        settings.webhook.url, VoltMod::Json::Write(payload),
        [](const VoltMod::HttpResult& result) {
            if (!result.Ok)
                Log::Warn("Webhook delivery failed: {}", result.Error);
        },
        {"Content-Type: application/json"}, RequestTimeoutMs);
}

}  // namespace Anticheat
