#include "CheatCheckRoomApi.hpp"

#include "../../Core/Config.hpp"

#include <CS2Kit/Utils/Json.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>
#include <map>
#include <nlohmann/json.hpp>

namespace AdminSystem::Admin::CheatCheck
{

using CS2Kit::Utils::Json;
using CS2Kit::Utils::StringUtils;

namespace
{
// Pull a field out of the response and, if a template is given, weave it in via {value}.
std::string BuildUrl(const nlohmann::json& json, const std::string& field, const std::string& tmpl)
{
    if (field.empty())
        return {};
    const std::string raw = Json::GetStringByPath(json, field);
    if (raw.empty())
        return {};
    return tmpl.empty() ? raw : StringUtils::SubstituteTokens(tmpl, {{"value", raw}});
}
}  // namespace

std::optional<RoomRequest> BuildRoomRequest(const Core::CheatCheckWebsiteAutoRoom& cfg, int64_t targetSteamId,
                                            std::string_view targetName, int64_t adminSteamId,
                                            std::string_view adminName)
{
    if (cfg.createRoomUrl.empty())
        return std::nullopt;

    const std::map<std::string, std::string> tokens = {
        {"steamId", std::to_string(targetSteamId)},
        {"playerName", std::string(targetName)},
        {"adminSteamId", std::to_string(adminSteamId)},
        {"adminName", std::string(adminName)},
    };

    nlohmann::json body = cfg.requestBody.is_null() ? nlohmann::json::object() : cfg.requestBody;
    Json::SubstituteTokens(body, tokens);

    std::vector<std::string> headers = {"Content-Type: application/json"};
    if (!cfg.apiKey.empty())
    {
        const std::string value = cfg.authScheme.empty() ? cfg.apiKey : cfg.authScheme + " " + cfg.apiKey;
        headers.push_back(cfg.authHeader + ": " + value);
    }

    return RoomRequest{
        .Url = cfg.createRoomUrl,
        .Body = body.dump(),
        .Headers = std::move(headers),
        .TimeoutMs = cfg.timeoutMs,
    };
}

std::optional<RoomUrls> ParseRoomResponse(const Core::CheatCheckWebsiteAutoRoom& cfg, const Web::HttpResult& result)
{
    if (!result.Ok || result.StatusCode < 200 || result.StatusCode >= 300)
        return std::nullopt;

    const auto json = nlohmann::json::parse(result.Body, nullptr, /*allow_exceptions=*/false);
    if (!json.is_structured())
        return std::nullopt;

    RoomUrls urls{
        .PlayerUrl = BuildUrl(json, cfg.playerUrlField, cfg.playerUrlTemplate),
        .CheckerUrl = BuildUrl(json, cfg.checkerUrlField, cfg.checkerUrlTemplate),
    };
    if (urls.PlayerUrl.empty())
        return std::nullopt;
    return urls;
}

}  // namespace AdminSystem::Admin::CheatCheck
