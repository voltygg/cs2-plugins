#pragma once

#include "../Punishments/PunishType.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace AdminSystem::Config
{

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

}  // namespace AdminSystem::Config
