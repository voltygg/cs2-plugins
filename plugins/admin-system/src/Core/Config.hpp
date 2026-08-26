#pragma once

#include "../Maps/MapQuery.hpp"
#include "../Punishments/PunishType.hpp"
#include "../Weapons/WeaponCatalog.hpp"

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

inline constexpr std::string_view AddonName = "admin-system";

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

/** Appeal route shown to a banned player on the disconnect screen. */
struct AppealSettings
{
    /** Appeal page. `{steamId}` is substituted; empty omits the appeal from the notice. */
    std::string url;
    /** Append how long the ban still has to run. */
    bool showExpiry = true;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AppealSettings, url, showExpiry)

struct PunishmentSettings
{
    std::string defaultBanReason = "Banned by administrator";
    int warningThreshold = 3;
    AppealSettings appeal;
    std::vector<PunishmentTemplate> templates;
    std::vector<std::string> reasonPresets;
    std::vector<std::string> menuDurations = {"5m", "30m", "1h", "1d", "7d", "perm"};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PunishmentSettings, defaultBanReason, warningThreshold, appeal,
                                                templates, reasonPresets, menuDurations)

/** Validated template used by the Quick Punish menu. */
struct ResolvedTemplate
{
    std::string Name;
    Punishments::PunishType Type = Punishments::PunishType::Ban;
    int DurationSec = 0;  // 0 = permanent
    std::string Reason;
};

/** Raw map entry, validated into Maps::MapEntry during load. */
struct MapConfigEntry
{
    std::string name;
    std::string displayName;
    uint64_t workshopId = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MapConfigEntry, name, displayName, workshopId)

/** The map vote an admin opens from the Map menu. */
struct MapVoteSettings
{
    /** Share of the ballots cast that must be yes; a strict majority of it is required. */
    double successRatio = 0.6;
    /** How long the panel stays open. */
    int durationSec = 20;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MapVoteSettings, successRatio, durationSec)

/** Maps an admin may switch to. The engine offers no usable list of its own, so this is it. */
struct MapSettings
{
    std::vector<MapConfigEntry> cycle;
    MapVoteSettings vote;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MapSettings, cycle, vote)

/** Raw weapon entry, validated into Weapons::WeaponEntry during load. */
struct WeaponConfigEntry
{
    std::string name;
    std::string item;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WeaponConfigEntry, name, item)

/** Weapons offered by `!give` and the weapon menu. */
struct WeaponSettings
{
    std::vector<WeaponConfigEntry> menu;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WeaponSettings, menu)

struct ChatSettings
{
    bool broadcastPunishments = true;

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
    MapSettings maps;
    WeaponSettings weapons;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Settings, plugin, server, database, punishments, abuseProtection, chat,
                                                reports, cheatCheck, maps, weapons)

/** Loads settings and resolves string-based punishment values. Admin and group
 *  records remain owned by the database. */
class ConfigManager : public VoltMod::JsonConfig<Settings>
{
public:
    ConfigManager() = default;

    /** Return false for missing, malformed, or mistyped settings. */
    bool LoadSettings(const std::string& path);

    const PluginSettings& GetPlugin() const { return Get().plugin; }
    const ServerSettings& GetServer() const { return Get().server; }
    const DatabaseConfig& GetDatabase() const { return Get().database; }
    const PunishmentSettings& GetPunishments() const { return Get().punishments; }
    const AppealSettings& GetAppeal() const { return Get().punishments.appeal; }
    const MapSettings& GetMaps() const { return Get().maps; }
    const AbuseProtectionSettings& GetAbuseProtection() const { return Get().abuseProtection; }
    const ChatSettings& GetChat() const { return Get().chat; }
    const ReportSettings& GetReports() const { return Get().reports; }
    const CheatCheckSettings& GetCheatCheck() const { return Get().cheatCheck; }

    /** Valid templates. Invalid entries are logged and skipped. */
    const std::vector<ResolvedTemplate>& GetPunishmentTemplates() const { return _resolvedTemplates; }

    /** Menu duration-picker rows in seconds (0 = permanent), parsed from `punishments.menuDurations`. */
    const std::vector<int>& GetMenuDurations() const { return _menuDurationSecs; }

    /** Offerable maps. Invalid entries are logged and skipped. */
    const std::vector<Maps::MapEntry>& GetMapCycle() const { return _resolvedMaps; }

    /** Giveable weapons. Invalid entries are logged and skipped. */
    const std::vector<Weapons::WeaponEntry>& GetWeaponMenu() const { return _resolvedWeapons; }

private:
    void ResolveRuntimeSettings();
    void ResolveMapCycle(const MapSettings& maps);
    void ResolveWeaponMenu(const WeaponSettings& weapons);

    std::vector<ResolvedTemplate> _resolvedTemplates;
    std::vector<int> _menuDurationSecs;
    std::vector<Maps::MapEntry> _resolvedMaps;
    std::vector<Weapons::WeaponEntry> _resolvedWeapons;
};

}  // namespace AdminSystem::Core
