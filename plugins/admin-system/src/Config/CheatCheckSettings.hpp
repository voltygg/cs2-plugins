#pragma once

#include <VoltMod/App/Config.hpp>
#include <string>

namespace AdminSystem::Config
{
struct CheatCheckFixedLink
{
    std::string url;
};

/** Create-room API settings; CheatCheckRoomApi defines the templating contract. The defaults
 *  describe the meat-app API, so pointing this at another one also means overriding the header,
 *  field and body keys. */
struct CheatCheckWebsiteAutoRoom
{
    std::string createRoomUrl;
    std::string apiKey;
    std::string authHeader = "X-API-Key";  // header name; Bearer APIs use "Authorization"
    std::string authScheme;                // value prefix; "" sends the key verbatim
    /** Body template, operator-authored and free-form; placeholders substituted per check. */
    glz::generic requestBody;
    std::string playerUrlField = "code";  // dot-path into the JSON response
    std::string playerUrlTemplate;        // {value} -> playerUrlField; empty uses the field as-is
    std::string checkerUrlField = "code";
    std::string checkerUrlTemplate;  // optional; relayed to the calling admin
    int timeoutMs = 8000;
    /** Presence URL. `{code}` and `{steamId}` are substituted; empty disables polling. */
    std::string presenceUrl;
    std::string presenceField = "present";  // dot-path to the response's in-room flag
    int pollIntervalSec = 5;                // also the worst-case delay before a join is noticed
};

struct CheatCheckSettings
{
    /** "fixedLink" | "websiteAutoRoom" | "playerProvided". */
    std::string mode = "fixedLink";
    int timeoutSec = 120;
    bool autoKick = true;
    std::string kickReason = "Failed to comply with cheat check";
    bool moveToSpectator = true;  // force the suspect to spectator so they can't keep playing
    std::string bannerImageUrl;   // optional online image shown atop the panel ("" => none)
    int bannerWidth = 320;
    int bannerHeight = 180;
    CheatCheckFixedLink fixedLink;
    CheatCheckWebsiteAutoRoom websiteAutoRoom;
};

}  // namespace AdminSystem::Config
