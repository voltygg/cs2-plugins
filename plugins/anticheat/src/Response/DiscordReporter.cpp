#include "DiscordReporter.hpp"

#include "App.hpp"

#include <VoltMod/Core/Json.hpp>
#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Strings.hpp>
#include <string>
#include <vector>

namespace Log = VoltMod::Log;

namespace Anticheat
{

static constexpr long RequestTimeoutMs = 5000;
static constexpr int EmbedColor = 0xE04F4F;

// Discord's embed shape. At namespace scope because reflection reads member names off the type.

struct DiscordField
{
    std::string name;
    std::string value;
    bool Inline = false;
};

struct DiscordEmbed
{
    std::string title;
    int color = 0;
    std::vector<DiscordField> fields;
};

struct DiscordPayload
{
    std::vector<DiscordEmbed> embeds;
};

}  // namespace Anticheat

/** `inline` is a keyword, so the wire key cannot be the member name. This has to precede the
 *  first write of a DiscordField. */
template <>
struct glz::meta<Anticheat::DiscordField>
{
    using T = Anticheat::DiscordField;
    static constexpr auto value = glz::object("name", &T::name, "value", &T::value, "inline", &T::Inline);
};

namespace Anticheat
{

void DiscordReporter::Report(int slot, const std::string& playerName, int64_t steamId, const Finding& finding,
                             FunnelOutcome outcome)
{
    const auto& settings = _config.Get().anticheat;
    if (settings.webhook.url.empty())
        return;
    if (!_throttle.TryAcquire({steamId, static_cast<int>(finding.Kind)}, VoltMod::Time::Now()))
        return;

    // Player names are client-controlled and need not be valid UTF-8. The writer passes bytes
    // through, so an unsanitized name would put invalid UTF-8 on the wire and Discord would
    // reject the whole delivery. Evidence can quote a name, so it goes through the same pass.
    const std::string safeName = VoltMod::Strings::SanitizeUtf8(playerName);
    const std::string safeEvidence = VoltMod::Strings::SanitizeUtf8(finding.Evidence);

    const DiscordPayload payload{
        .embeds = {DiscordEmbed{
            .title = std::string(DisplayName(finding.Kind)),
            .color = EmbedColor,
            .fields =
                {
                    {.name = "Player", .value = safeName.empty() ? "<unknown>" : safeName, .Inline = true},
                    {.name = "SteamID64", .value = steamId ? std::to_string(steamId) : "unavailable", .Inline = true},
                    {.name = "Slot", .value = std::to_string(slot), .Inline = true},
                    {.name = "Outcome", .value = std::string(OutcomeName(outcome)), .Inline = true},
                    {.name = "Mode", .value = std::string(ModeName(ParseMode(settings.mode))), .Inline = true},
                    {.name = "Evidence", .value = safeEvidence.empty() ? "-" : safeEvidence},
                },
        }},
    };

    _rt.Http.Post(
        settings.webhook.url, VoltMod::Json::Write(payload),
        [](const VoltMod::HttpResult& result) {
            if (!result.Ok)
                Log::Warn("Webhook delivery failed: {}", result.Error);
        },
        {"Content-Type: application/json"}, RequestTimeoutMs);
}

}  // namespace Anticheat
