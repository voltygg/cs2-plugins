#include "CheatCheckRoomApi.hpp"

#include "../../Core/Config.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Strings.hpp>
#include <map>
#include <utility>

namespace AdminSystem::Admin::CheatCheck
{

using VoltMod::Http::BuildJsonPost;
using VoltMod::Http::ExtractField;
using VoltMod::Http::IsSuccess;
using VoltMod::Http::JsonPostSpec;

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

std::optional<RoomUrls> ParseRoomResponse(const Core::CheatCheckWebsiteAutoRoom& cfg, const VoltMod::HttpResult& result)
{
    if (!IsSuccess(result))
        return std::nullopt;

    std::string code = ExtractField(result, cfg.playerUrlField);
    if (code.empty())
        return std::nullopt;

    using VoltMod::Core::Strings;
    return RoomUrls{
        .PlayerUrl = cfg.playerUrlTemplate.empty()
                         ? code
                         : Strings::SubstituteTokens(cfg.playerUrlTemplate, {{"value", code}}),
        .CheckerUrl = ExtractField(result, cfg.checkerUrlField, cfg.checkerUrlTemplate),
        .RoomCode = std::move(code),
    };
}

std::optional<RoomRequest> BuildPresenceRequest(const Core::CheatCheckWebsiteAutoRoom& cfg, const std::string& roomCode,
                                                int64_t targetSteamId)
{
    if (cfg.presenceUrl.empty() || roomCode.empty())
        return std::nullopt;

    VoltMod::Http::JsonGetSpec spec{
        .UrlTemplate = cfg.presenceUrl,
        .ApiKey = cfg.apiKey,
        .AuthHeader = cfg.authHeader,
        .AuthScheme = cfg.authScheme,
        .TimeoutMs = cfg.timeoutMs,
    };
    return VoltMod::Http::BuildJsonGet(spec, {
                                                 {"code", roomCode},
                                                 {"steamId", std::to_string(targetSteamId)},
                                             });
}

std::optional<bool> ParsePresence(const Core::CheatCheckWebsiteAutoRoom& cfg, const VoltMod::HttpResult& result)
{
    if (!IsSuccess(result))
        return std::nullopt;

    const std::string present = ExtractField(result, cfg.presenceField);
    if (present.empty())
        return std::nullopt;
    return present == "true";
}

}  // namespace AdminSystem::Admin::CheatCheck
