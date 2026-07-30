#pragma once

#include "../Punishments/PunishType.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Database/PostgresDatabase.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace CS2Kit::Database
{
// PostgresConfig's mapper must share its namespace so ADL finds it. Defined here rather than
// in the kit so the kit header stays nlohmann-free.
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PostgresConfig, host, port, database, username, password, sslMode)
}  // namespace CS2Kit::Database

namespace AdminSystem::Core
{

using DatabaseConfig = CS2Kit::Database::PostgresConfig;

/** Kit-standard "plugin" section of settings.json (locale). */
using PluginSettings = CS2Kit::StandardPluginSettings;

/** "server" section of settings.jsonc: this server's identity in the shared database.
 *  The tag keys per-server admin grants (admin_server_groups) and audit attribution, so it
 *  must be unique per server and stable once referenced. */
struct ServerSettings
{
    std::string tag = "default";
    std::string name = "CS2 Server";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ServerSettings, tag, name)

/** "abuseProtection" section of settings.jsonc: automatic admin-freeze rate thresholds.
 *  A threshold of 0 disables that counter; root ('z') admins are always exempt. */
struct AbuseProtectionSettings
{
    bool enabled = true;
    int windowMinutes = 10;
    int maxBans = 5;
    int maxKicks = 10;
    int maxMutes = 15;  // voice + text combined
    int maxWarnings = 15;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AbuseProtectionSettings, enabled, windowMinutes, maxBans, maxKicks,
                                                maxMutes, maxWarnings)

/** One "punishments.templates[]" entry, verbatim from JSON. Validated into a ResolvedTemplate on load. */
struct PunishmentTemplate
{
    std::string name;
    std::string type;      // "ban" | "voiceMute" | "textMute"
    std::string duration;  // CS2Kit::ParseDuration grammar: 30s/5m/2h/7d, "perm"/"0" = permanent
    std::string reason;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PunishmentTemplate, name, type, duration, reason)

/** "punishments" section of settings.json. */
struct PunishmentSettings
{
    std::string defaultBanReason = "Banned by administrator";
    int warningThreshold = 3;
    std::vector<PunishmentTemplate> templates;
    std::vector<std::string> reasonPresets;
    std::vector<std::string> menuDurations = {"5m", "30m", "1h", "1d", "7d", "perm"};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PunishmentSettings, defaultBanReason, warningThreshold, templates,
                                                reasonPresets, menuDurations)

/** A punishment template that survived load-time validation; what the Quick Punish menu consumes. */
struct ResolvedTemplate
{
    std::string Name;
    Punishments::PunishType Type = Punishments::PunishType::Ban;
    int DurationSec = 0;  // 0 = permanent
    std::string Reason;
};

/** "chat" section of settings.json. */
struct ChatSettings
{
    /** If true, every issued punishment broadcasts a colored line to all players. */
    bool broadcastPunishments = true;

    /** If true, an admin's chat is intercepted and re-emitted with their group's colored prefix. */
    bool tagAdminChatMessages = true;

    /** Prefix used when an admin doesn't belong to any group with a prefix set. */
    std::string fallbackPrefix = "[ADMIN]";
    std::string fallbackPrefixColor = "red";
    std::string fallbackNameColor = "default";
    std::string fallbackMessageColor = "default";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ChatSettings, broadcastPunishments, tagAdminChatMessages,
                                                fallbackPrefix, fallbackPrefixColor, fallbackNameColor,
                                                fallbackMessageColor)

/** One "reports.reasons[]" entry. The menu prefers the `report.reasons.<code>` translation, so
 *  `label` only surfaces for codes the translation files don't cover. */
struct ReportReason
{
    std::string code;
    std::string label;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ReportReason, code, label)

/** "reports" section of settings.jsonc: player-to-player reporting (!r / !report). Rows go to
 *  `player_reports` for an upstream website; nothing is surfaced in-game. */
struct ReportSettings
{
    bool enabled = true;
    /** Seconds between two reports of ANY player. 0 disables. */
    int cooldownSec = 120;
    /** Seconds before the same reporter may re-report the SAME player. 0 disables. */
    int duplicateWindowSec = 1800;
    /** Adds an "Other..." row storing typed text under the "other" code (capped at 64 chars). */
    bool allowCustomReason = true;
    std::vector<ReportReason> reasons = {
        {"cheating", "Cheating / aimbot"}, {"wallhack", "Wallhack"}, {"griefing", "Griefing / team damage"},
        {"abuse", "Toxic behavior"},       {"micspam", "Mic spam"},  {"nickname", "Inappropriate nickname"},
    };
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ReportSettings, enabled, cooldownSec, duplicateWindowSec,
                                                allowCustomReason, reasons)

/** "cheatCheck.fixedLink" sub-section. */
struct CheatCheckFixedLink
{
    std::string url;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CheatCheckFixedLink, url)

/** "cheatCheck.websiteAutoRoom" sub-section: a backend-agnostic create-room API. See the field
 *  comments and CheatCheckRoomApi for the request/response templating contract. */
struct CheatCheckWebsiteAutoRoom
{
    std::string createRoomUrl;
    std::string apiKey;
    std::string authHeader = "Authorization";  // header name; e.g. "X-API-Key"
    std::string authScheme = "Bearer";         // value prefix; "" sends the key verbatim
    nlohmann::json requestBody;                // body template, placeholders substituted per check
    std::string playerUrlField = "playerUrl";  // dot-path into the JSON response
    std::string playerUrlTemplate;             // {value} -> playerUrlField; empty uses the field as-is
    std::string checkerUrlField = "checkerUrl";
    std::string checkerUrlTemplate;  // optional; relayed to the calling admin
    int timeoutMs = 8000;
    /** Presence poll GET; {code} receives the raw playerUrlField value, {steamId} the suspect's
     *  SteamID64. "" disables polling (the countdown then never pauses). */
    std::string presenceUrl;
    std::string presenceField = "present";  // dot-path to the response's in-room flag
    int pollIntervalSec = 5;                // also the worst-case delay before a join is noticed
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CheatCheckWebsiteAutoRoom, createRoomUrl, apiKey, authHeader,
                                                authScheme, requestBody, playerUrlField, playerUrlTemplate,
                                                checkerUrlField, checkerUrlTemplate, timeoutMs, presenceUrl,
                                                presenceField, pollIntervalSec)

/** "cheatCheck" section of settings.jsonc. */
struct CheatCheckSettings
{
    /** "fixedLink" | "websiteAutoRoom" | "playerProvided". */
    std::string mode = "fixedLink";
    int timeoutSec = 120;
    bool autoKick = true;
    std::string kickReason = "Failed to comply with cheat check";
    // The client drops center-HTML almost immediately (death, team switch, HUD updates), so the
    // panel must be re-sent continuously like the WASD menus - not at the nominal 5s duration.
    int panelRefreshMs = 100;
    bool moveToSpectator = true;  // force the suspect to spectator so they can't keep playing
    std::string bannerImageUrl;   // optional online image shown atop the panel ("" => none)
    int bannerWidth = 320;
    int bannerHeight = 180;
    CheatCheckFixedLink fixedLink;
    CheatCheckWebsiteAutoRoom websiteAutoRoom;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CheatCheckSettings, mode, timeoutSec, autoKick, kickReason,
                                                panelRefreshMs, moveToSpectator, bannerImageUrl, bannerWidth,
                                                bannerHeight, fixedLink, websiteAutoRoom)

/** Root of settings.jsonc. Mirrors the JSON object so the whole file deserializes in one call. */
struct Settings
{
    PluginSettings plugin;
    ServerSettings server;
    DatabaseConfig database;
    PunishmentSettings punishments;
    AbuseProtectionSettings abuseProtection;
    ChatSettings chat;
    ReportSettings reports;
    CheatCheckSettings cheatCheck;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Settings, plugin, server, database, punishments, abuseProtection, chat,
                                                reports, cheatCheck)

/**
 * Loads and owns settings.json (via the kit's JsonConfig). All admin/group data is owned by the
 * database (`admins` and `admin_groups` tables) - this manager only exposes plugin/DB/punishment/
 * chat config plus the validated runtime forms of the string-typed punishment settings.
 */
class ConfigManager : public CS2Kit::JsonConfig<Settings>
{
public:
    ConfigManager() = default;

    /** Load settings.json, then validate/resolve the runtime forms. Returns false if the file
     *  is missing, unparseable, or has a wrong-typed value. */
    bool LoadSettings(const std::string& path);

    const PluginSettings& GetPlugin() const { return Get().plugin; }
    const ServerSettings& GetServer() const { return Get().server; }
    const DatabaseConfig& GetDatabase() const { return Get().database; }
    const PunishmentSettings& GetPunishments() const { return Get().punishments; }
    const AbuseProtectionSettings& GetAbuseProtection() const { return Get().abuseProtection; }
    const ChatSettings& GetChat() const { return Get().chat; }
    const ReportSettings& GetReports() const { return Get().reports; }
    const CheatCheckSettings& GetCheatCheck() const { return Get().cheatCheck; }

    /** Punishment templates that passed load-time validation (invalid entries are skipped with a warning). */
    const std::vector<ResolvedTemplate>& GetPunishmentTemplates() const { return _resolvedTemplates; }

    /** Menu duration-picker rows in seconds (0 = permanent), parsed from `punishments.menuDurations`. */
    const std::vector<int>& GetMenuDurations() const { return _menuDurationSecs; }

private:
    /** Parse/validate the string-typed punishment settings into their runtime forms. */
    void ResolveRuntimeSettings();

    std::vector<ResolvedTemplate> _resolvedTemplates;
    std::vector<int> _menuDurationSecs;
};

}  // namespace AdminSystem::Core
