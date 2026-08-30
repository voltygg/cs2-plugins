#pragma once

#include "../../Config/CheatCheckSettings.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Http/HttpClient.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace AdminSystem::Admin::CheatCheck
{

/** URLs extracted from a successful create-room response. */
struct RoomUrls
{
    std::string PlayerUrl;   // shown to the suspect
    std::string CheckerUrl;  // optional; relayed to the admin
    std::string RoomCode;    // raw playerUrlField value; keys the presence polling
};

/**
 * Build the create-room POST from the websiteAutoRoom config. Substitutes
 * {steamId}/{playerName}/{adminSteamId}/{adminName} into the body template and assembles the auth
 * header. Returns nullopt when there is no endpoint configured.
 */
std::optional<VoltMod::HttpRequest> BuildRoomRequest(const Config::CheatCheckWebsiteAutoRoom& cfg,
                                                     int64_t targetSteamId, std::string_view targetName,
                                                     int64_t adminSteamId, std::string_view adminName);

/**
 * Parse the create-room response into player/checker URLs per the config's field + template rules.
 * Returns nullopt on transport/HTTP/parse failure or when no player URL could be resolved.
 */
std::optional<RoomUrls> ParseRoomResponse(const Config::CheatCheckWebsiteAutoRoom& cfg,
                                          const VoltMod::HttpResult& result);

/**
 * Build the presence GET from the websiteAutoRoom config. Substitutes {code}/{steamId} into the
 * URL template. Returns nullopt when polling is not configured or @p roomCode is empty.
 */
std::optional<VoltMod::HttpRequest> BuildPresenceRequest(const Config::CheatCheckWebsiteAutoRoom& cfg,
                                                         const std::string& roomCode, int64_t targetSteamId);

/**
 * Read the in-room flag from a presence response. Returns nullopt on transport/HTTP/parse
 * failure so callers can distinguish "not in the room" from "API broke".
 */
std::optional<bool> ParsePresence(const Config::CheatCheckWebsiteAutoRoom& cfg, const VoltMod::HttpResult& result);

}  // namespace AdminSystem::Admin::CheatCheck
