#pragma once

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Http/RestJsonApi.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace AdminSystem::Core
{
struct CheatCheckWebsiteAutoRoom;
}

namespace AdminSystem::Admin::CheatCheck
{

/** A ready-to-send create-room HTTP POST, derived from config + the players involved. */
using RoomRequest = CS2Kit::Http::PreparedRequest;

/** URLs extracted from a successful create-room response. */
struct RoomUrls
{
    std::string PlayerUrl;   // shown to the suspect
    std::string CheckerUrl;  // optional; relayed to the admin
};

/**
 * Build the create-room POST from the websiteAutoRoom config. Substitutes
 * {steamId}/{playerName}/{adminSteamId}/{adminName} into the body template and assembles the auth
 * header. Returns nullopt when there is no endpoint configured.
 */
std::optional<RoomRequest> BuildRoomRequest(const Core::CheatCheckWebsiteAutoRoom& cfg, int64_t targetSteamId,
                                            std::string_view targetName, int64_t adminSteamId,
                                            std::string_view adminName);

/**
 * Parse the create-room response into player/checker URLs per the config's field + template rules.
 * Returns nullopt on transport/HTTP/parse failure or when no player URL could be resolved.
 */
std::optional<RoomUrls> ParseRoomResponse(const Core::CheatCheckWebsiteAutoRoom& cfg,
                                          const CS2Kit::HttpResult& result);

}  // namespace AdminSystem::Admin::CheatCheck
