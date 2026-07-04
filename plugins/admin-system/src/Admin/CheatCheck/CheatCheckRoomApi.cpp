#include "CheatCheckRoomApi.hpp"

#include "../../Core/Config.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>
#include <map>
#include <utility>

namespace AdminSystem::Admin::CheatCheck
{

using CS2Kit::Http::BuildJsonPost;
using CS2Kit::Http::ExtractField;
using CS2Kit::Http::IsSuccess;
using CS2Kit::Http::JsonPostSpec;

std::optional<RoomRequest> BuildRoomRequest(const Core::CheatCheckWebsiteAutoRoom& cfg, int64_t targetSteamId,
                                            std::string_view targetName, int64_t adminSteamId,
                                            std::string_view adminName)
{
    JsonPostSpec spec{
        .Url = cfg.createRoomUrl,
        .ApiKey = cfg.apiKey,
        .AuthHeader = cfg.authHeader,
        .AuthScheme = cfg.authScheme,
        .BodyTemplate = cfg.requestBody,
        .TimeoutMs = cfg.timeoutMs,
    };
    return BuildJsonPost(spec, {
                                   {"steamId", std::to_string(targetSteamId)},
                                   {"playerName", std::string(targetName)},
                                   {"adminSteamId", std::to_string(adminSteamId)},
                                   {"adminName", std::string(adminName)},
                               });
}

std::optional<RoomUrls> ParseRoomResponse(const Core::CheatCheckWebsiteAutoRoom& cfg,
                                          const CS2Kit::HttpResult& result)
{
    if (!IsSuccess(result))
        return std::nullopt;

    std::string code = ExtractField(result, cfg.playerUrlField);
    if (code.empty())
        return std::nullopt;

    using CS2Kit::Utils::StringUtils;
    return RoomUrls{
        .PlayerUrl = cfg.playerUrlTemplate.empty() ? code
                                                   : StringUtils::SubstituteTokens(cfg.playerUrlTemplate,
                                                                                   {{"value", code}}),
        .CheckerUrl = ExtractField(result, cfg.checkerUrlField, cfg.checkerUrlTemplate),
        .RoomCode = std::move(code),
    };
}

std::optional<RoomRequest> BuildPresenceRequest(const Core::CheatCheckWebsiteAutoRoom& cfg,
                                                const std::string& roomCode, int64_t targetSteamId)
{
    if (cfg.presenceUrl.empty() || roomCode.empty())
        return std::nullopt;

    CS2Kit::Http::JsonGetSpec spec{
        .UrlTemplate = cfg.presenceUrl,
        .ApiKey = cfg.apiKey,
        .AuthHeader = cfg.authHeader,
        .AuthScheme = cfg.authScheme,
        .TimeoutMs = cfg.timeoutMs,
    };
    return CS2Kit::Http::BuildJsonGet(spec, {
                                                {"code", roomCode},
                                                {"steamId", std::to_string(targetSteamId)},
                                            });
}

std::optional<bool> ParsePresence(const Core::CheatCheckWebsiteAutoRoom& cfg, const CS2Kit::HttpResult& result)
{
    if (!IsSuccess(result))
        return std::nullopt;

    const std::string present = ExtractField(result, cfg.presenceField);
    if (present.empty())
        return std::nullopt;
    return present == "true";
}

}  // namespace AdminSystem::Admin::CheatCheck
