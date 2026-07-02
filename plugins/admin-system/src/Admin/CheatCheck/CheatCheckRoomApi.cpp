#include "CheatCheckRoomApi.hpp"

#include "../../Core/Config.hpp"

#include <map>

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
                                          const CS2Kit::Http::HttpResult& result)
{
    if (!IsSuccess(result))
        return std::nullopt;

    RoomUrls urls{
        .PlayerUrl = ExtractField(result, cfg.playerUrlField, cfg.playerUrlTemplate),
        .CheckerUrl = ExtractField(result, cfg.checkerUrlField, cfg.checkerUrlTemplate),
    };
    if (urls.PlayerUrl.empty())
        return std::nullopt;
    return urls;
}

}  // namespace AdminSystem::Admin::CheatCheck
