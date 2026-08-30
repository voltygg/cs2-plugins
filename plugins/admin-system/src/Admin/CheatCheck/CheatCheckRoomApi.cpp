#include "CheatCheckRoomApi.hpp"

#include "../../Config/CheatCheckSettings.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Json.hpp>
#include <VoltMod/Core/Strings.hpp>
#include <map>
#include <utility>

namespace AdminSystem::Admin::CheatCheck
{

static std::string ExtractField(const VoltMod::HttpResult& result, std::string_view path,
                                const std::string& valueTemplate = {})
{
    if (path.empty())
        return {};

    const auto json = nlohmann::json::parse(result.Body, nullptr, /*allow_exceptions=*/false);
    if (!json.is_structured())
        return {};

    const std::string value = VoltMod::Json::GetStringByPath(json, path);
    if (value.empty() || valueTemplate.empty())
        return value;
    return VoltMod::Strings::SubstituteTokens(valueTemplate, {{"value", value}});
}

std::optional<VoltMod::HttpRequest> BuildRoomRequest(const Config::CheatCheckWebsiteAutoRoom& cfg,
                                                     int64_t targetSteamId, std::string_view targetName,
                                                     int64_t adminSteamId, std::string_view adminName)
{
    if (cfg.createRoomUrl.empty())
        return std::nullopt;

    nlohmann::json body = cfg.requestBody.is_null() ? nlohmann::json::object() : cfg.requestBody;
    VoltMod::Json::SubstituteTokens(body, {
                                              {"steamId", std::to_string(targetSteamId)},
                                              {"playerName", std::string(targetName)},
                                              {"adminSteamId", std::to_string(adminSteamId)},
                                              {"adminName", std::string(adminName)},
                                          });

    VoltMod::HttpRequest request{
        .Method = VoltMod::HttpMethod::Post,
        .Url = cfg.createRoomUrl,
        .Body = body.dump(),
        .TimeoutMs = cfg.timeoutMs,
    };
    request.AddHeader("Content-Type", "application/json");
    request.AddAuth(cfg.authHeader, cfg.authScheme, cfg.apiKey);
    return request;
}

std::optional<RoomUrls> ParseRoomResponse(const Config::CheatCheckWebsiteAutoRoom& cfg,
                                          const VoltMod::HttpResult& result)
{
    if (!result.IsSuccess())
        return std::nullopt;

    std::string code = ExtractField(result, cfg.playerUrlField);
    if (code.empty())
        return std::nullopt;

    using VoltMod::Strings;
    return RoomUrls{
        .PlayerUrl =
            cfg.playerUrlTemplate.empty() ? code : Strings::SubstituteTokens(cfg.playerUrlTemplate, {{"value", code}}),
        .CheckerUrl = ExtractField(result, cfg.checkerUrlField, cfg.checkerUrlTemplate),
        .RoomCode = std::move(code),
    };
}

std::optional<VoltMod::HttpRequest> BuildPresenceRequest(const Config::CheatCheckWebsiteAutoRoom& cfg,
                                                         const std::string& roomCode, int64_t targetSteamId)
{
    if (cfg.presenceUrl.empty() || roomCode.empty())
        return std::nullopt;

    VoltMod::HttpRequest request{
        .Method = VoltMod::HttpMethod::Get,
        .Url = VoltMod::Strings::SubstituteTokens(cfg.presenceUrl,
                                                  {
                                                      {"code", roomCode},
                                                      {"steamId", std::to_string(targetSteamId)},
                                                  }),
        .TimeoutMs = cfg.timeoutMs,
    };
    request.AddAuth(cfg.authHeader, cfg.authScheme, cfg.apiKey);
    return request;
}

std::optional<bool> ParsePresence(const Config::CheatCheckWebsiteAutoRoom& cfg, const VoltMod::HttpResult& result)
{
    if (!result.IsSuccess())
        return std::nullopt;

    const std::string present = ExtractField(result, cfg.presenceField);
    if (present.empty())
        return std::nullopt;
    return present == "true";
}

}  // namespace AdminSystem::Admin::CheatCheck
