#include "DiscordReporter.hpp"

#include "App.hpp"

#include <VoltMod/Core/Log.hpp>
#include <nlohmann/json.hpp>

namespace Log = VoltMod::Core::Log;

namespace Anticheat
{

namespace
{
constexpr long RequestTimeoutMs = 5000;
constexpr int EmbedColor = 0xE04F4F;
}  // namespace

void DiscordReporter::Report(int slot, const std::string& playerName, int64_t steamId, const Finding& finding,
                             FunnelOutcome outcome)
{
    const auto& settings = _config.Get().anticheat;
    if (settings.webhook.url.empty())
        return;
    if (!_throttle.TryAcquire({steamId, static_cast<int>(finding.Kind)}, VoltMod::TimeUtils::Now()))
        return;

    nlohmann::json embed{
        {"title", DisplayName(finding.Kind)},
        {"color", EmbedColor},
        {"fields",
         nlohmann::json::array({
             {{"name", "Player"}, {"value", playerName.empty() ? "<unknown>" : playerName}, {"inline", true}},
             {{"name", "SteamID64"}, {"value", steamId ? std::to_string(steamId) : "unavailable"}, {"inline", true}},
             {{"name", "Slot"}, {"value", std::to_string(slot)}, {"inline", true}},
             {{"name", "Outcome"}, {"value", OutcomeName(outcome)}, {"inline", true}},
             {{"name", "Mode"}, {"value", ModeName(ParseMode(settings.mode))}, {"inline", true}},
             {{"name", "Evidence"}, {"value", finding.Evidence.empty() ? "-" : finding.Evidence}},
         })},
    };
    const nlohmann::json payload{{"embeds", nlohmann::json::array({embed})}};

    // Player names are client-controlled and need not be valid UTF-8, which the default dump()
    // throws on - and this runs on an engine frame with nothing to catch it.
    std::string body = payload.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);

    _rt.Http.Post(settings.webhook.url, std::move(body), {"Content-Type: application/json"}, RequestTimeoutMs,
                  [](const VoltMod::HttpResult& result) {
                      if (!result.Ok)
                          Log::Warn("Webhook delivery failed: {}", result.Error);
                  });
}

}  // namespace Anticheat
