#include "Config.hpp"

#include <CS2Kit/Utils/Validation.hpp>
#include <algorithm>
#include <format>
#include <optional>

namespace AdminSystem::Core
{

using namespace CS2Kit::Utils;

bool ConfigManager::LoadSettings(const std::string& path)
{
    if (!Load(path))
        return false;

    ResolveRuntimeSettings();
    return true;
}

// Bad entries are skipped with a warning rather than failing the load: a typo'd template
// must not take down ban/mute enforcement for the whole server.
void ConfigManager::ResolveRuntimeSettings()
{
    auto& settings = Mutable();

    // A blank/oversized tag would silently orphan per-server grants (the DB column is VARCHAR(64)).
    Validation::NormalizeTag(settings.server.tag, 64, "default", "server.tag");

    auto& punishments = settings.punishments;
    Validation::FilterValid(
        punishments.templates,
        [](const PunishmentTemplate& t, std::size_t) -> std::optional<std::string> {
            auto type = Punishments::ParsePunishType(t.type);
            if (!type || !Punishments::IsTimed(*type))
                return std::format("type must be ban/voiceMute/textMute, got '{}'", t.type);
            if (ParseDuration(t.duration) < 0)
                return std::format("bad duration '{}'", t.duration);
            if (t.name.empty() || t.reason.empty())
                return std::string("name and reason must be non-empty");
            return std::nullopt;
        },
        "punishments.templates");

    _resolvedTemplates.clear();
    for (const auto& t : punishments.templates)
        _resolvedTemplates.push_back(
            {t.name, *Punishments::ParsePunishType(t.type), ParseDuration(t.duration), t.reason});

    // An empty duration picker would dead-end the menu ban/mute flow; the helper falls back to
    // the struct defaults so the list exists in exactly one place.
    _menuDurationSecs = Validation::ParseDurations(punishments.menuDurations, PunishmentSettings{}.menuDurations,
                                                   "punishments.menuDurations");

    auto& reports = settings.reports;
    Validation::FilterValid(
        reports.reasons,
        [](const ReportReason& r, std::size_t) -> std::optional<std::string> {
            if (r.code.empty() || r.label.empty())
                return std::string("code and label must be non-empty");
            if (r.code.size() > 32)  // the reason_code column is VARCHAR(32)
                return std::format("code '{}' is longer than 32 chars", r.code);
            return std::nullopt;
        },
        "reports.reasons");

    // A reason picker with only the free-text row (or nothing at all) is a dead end.
    Validation::FallbackIfEmpty(reports.reasons, ReportSettings{}.reasons, "reports.reasons");

    // Negative windows would make every elapsed check pass; treat them as "disabled".
    reports.cooldownSec = std::max(reports.cooldownSec, 0);
    reports.duplicateWindowSec = std::max(reports.duplicateWindowSec, 0);
}

}  // namespace AdminSystem::Core
