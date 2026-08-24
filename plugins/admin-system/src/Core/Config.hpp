#pragma once

#include "../Punishments/PunishType.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Database/PostgresDatabase.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace VoltMod::Database
{
// ADL requires this mapper in PostgresConfig's namespace. Keeping it here also
// keeps nlohmann out of the framework header.
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PostgresConfig, host, port, database, username, password, sslMode)
}  // namespace VoltMod::Database

namespace AdminSystem::Core
{

using DatabaseConfig = VoltMod::Database::PostgresConfig;

/** Addon directory and CMake target name. */
inline constexpr std::string_view AddonName = "admin-system";

/** Framework `plugin` settings, including the locale. */
using PluginSettings = VoltMod::StandardPluginSettings;

/** Server identity in the shared database.
 *  The tag keys grants and audit records, so it must remain unique and stable. */
struct ServerSettings
{
    std::string tag = "default";
    std::string name = "CS2 Server";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ServerSettings, tag, name)

/** Automatic admin-freeze thresholds. Zero disables a counter; root admins are exempt. */
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

/** Raw punishment template, validated into ResolvedTemplate during load. */
struct PunishmentTemplate
{
    std::string name;
    std::string type;      // "ban" | "voiceMute" | "textMute"
    std::string duration;  // VoltMod::ParseDuration grammar: 30s/5m/2h/7d, "perm"/"0" = permanent
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

/** Validated template used by the Quick Punish menu. */
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
    /** Broadcast issued punishments to all players. */
    bool broadcastPunishments = true;

    /** Re-emit admin chat with the group's colored prefix. */
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

/** Report reason. `label` is the fallback for a missing `report.reasons.<code>` translation. */
struct ReportReason
{
    std::string code;
    std::string label;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ReportReason, code, label)

/** Player reports stored for the website; the plugin has no in-game report list. */
struct ReportSettings
{
    bool enabled = true;
    /** Seconds between reports by one player. Zero disables the limit. */
    int cooldownSec = 120;
    /** Seconds before one player may report the same target again. Zero disables the limit. */
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

/** Create-room API settings. CheatCheckRoomApi defines the templating contract. */
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
    /** Presence URL. `{code}` and `{steamId}` are substituted; empty disables polling. */
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
    // Death, team changes, and HUD updates dismiss center HTML, so refresh it
    // more often than its nominal five-second lifetime.
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

/** Root settings object. */
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

/** Loads settings and resolves string-based punishment values. Admin and group
 *  records remain owned by the database. */
class ConfigManager : public VoltMod::JsonConfig<Settings>
{
public:
    ConfigManager() = default;

    /** Load and validate settings. Returns false for missing, malformed, or mistyped input. */
    bool LoadSettings(const std::string& path);

    const PluginSettings& GetPlugin() const { return Get().plugin; }
    const ServerSettings& GetServer() const { return Get().server; }
    const DatabaseConfig& GetDatabase() const { return Get().database; }
    const PunishmentSettings& GetPunishments() const { return Get().punishments; }
    const AbuseProtectionSettings& GetAbuseProtection() const { return Get().abuseProtection; }
    const ChatSettings& GetChat() const { return Get().chat; }
    const ReportSettings& GetReports() const { return Get().reports; }
    const CheatCheckSettings& GetCheatCheck() const { return Get().cheatCheck; }

    /** Valid templates. Invalid entries are logged and skipped. */
    const std::vector<ResolvedTemplate>& GetPunishmentTemplates() const { return _resolvedTemplates; }

    /** Menu duration-picker rows in seconds (0 = permanent), parsed from `punishments.menuDurations`. */
    const std::vector<int>& GetMenuDurations() const { return _menuDurationSecs; }

private:
    /** Parse string-based punishment settings into runtime values. */
    void ResolveRuntimeSettings();

    std::vector<ResolvedTemplate> _resolvedTemplates;
    std::vector<int> _menuDurationSecs;
};

}  // namespace AdminSystem::Core
